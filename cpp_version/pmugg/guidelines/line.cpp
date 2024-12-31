#include "line.h"
#include "line_segment.h"
#include "endpoint.h"
#include "edge_type.h"
#include "vertex.h"
#include "face.h"
#include "brush.h"
#include "util.h"
#include "settings.h"
#include <algorithm>

namespace ms {

std::unordered_map<int, VertexType*> Line::splitVertexTypes;

Line::Line(Stats* stats, EdgeType* edgeType)
    : edgeType(edgeType)
    , tileSeeds{Util::random(100), Util::random(100)} {
    
    int numEndpoints = globalSettings.getBool("Use Network") ? 
                      edgeType->getFaceData().size() : 2;

    std::map<std::string, std::unique_ptr<Property>> properties;
    properties["lineSegment"] = std::make_unique<RequiredArray>("lineSegment");
    properties["endpoint"] = std::make_unique<RequiredArray>("endpoint", false, numEndpoints);
    properties["ringInstance"] = std::make_unique<AlternativeArray>("ringInstance", false);
    properties["cost"] = std::make_unique<ValueProperty>("cost", "lineDistance");

    node = std::make_unique<Node>(this, stats, "line", std::move(properties));
    setCost(0);
}

Node* Line::getNode() const {
    return node.get();
}

std::vector<LineSegment*> Line::getSegments() const {
    return node->get("lineSegment");
}

std::vector<RingInstance*> Line::getRingInstances() const {
    return node->get("ringInstance");
}

std::vector<Endpoint*> Line::getEndpoints() const {
    return node->get("endpoint");
}

LineSegment* Line::getSegment(bool isAtStart) const {
    auto segments = getSegments();
    return segments[isAtStart ? 0 : segments.size() - 1];
}

LineState* Line::getState(bool isAtStart) const {
    auto states = getSegment(isAtStart)->getStates();
    return states[isAtStart ? 0 : states.size() - 1];
}

EdgeType* Line::getEdgeType() const {
    return edgeType;
}

float Line::getCost() const {
    return node->get("cost");
}

void Line::setCost(float cost) {
    node->setValue("cost", cost);
}

const Line::TileSeeds& Line::getTileSeeds() const {
    return tileSeeds;
}

Line* Line::copy() const {
    return new Line(node->getStats(), edgeType);
}

void Line::addSegments(const std::vector<LineSegment*>& segments, bool atStart) {
    for (auto* segment : segments) {
        node->connect(segment, atStart);
    }
}

void Line::addEndpoint(Endpoint* endpoint, int index) {
    node->insert(endpoint, index);
}

bool Line::isRigid() const {
    auto endpoints = getEndpoints();
    return (endpoints[0] && endpoints[0]->isRigid()) ||
           (endpoints[1] && endpoints[1]->isRigid());
}

Line::SplitResult Line::split() {
    timer->start("split No Vertex");
    auto stats = node->getStats();

    auto newLines = std::vector<Line*>{copy(), copy()};
    newLines[0]->addSegments({new LineSegment(stats)});
    newLines[1]->addSegments({new LineSegment(stats)});

    auto endpoints = getEndpoints();
    auto nextEndpoints = endpoints;
    for (auto* endpoint : endpoints) {
        endpoint->getFace()->split(endpoint->next());
    }
    for (auto* endpoint : endpoints) {
        endpoint->transfer(newLines[endpoint->getIsAtStart() ? 0 : 1]);
    }
    node->destroy();
    timer->stop("split No Vertex");
    return {newLines, nextEndpoints};
}

Brush* Line::getBrush() const {
    return edgeType->getBrush();
}

void Line::fillFromEndpoints(bool addToModel) {
    auto endpoints = getEndpoints();
    
    auto endpoint0 = endpoints[0];
    auto endpoint1 = endpoints[1] ? endpoints[1] : endpoint0->next();
    auto p0 = endpoint0->getPosition();
    auto p1 = endpoint1->getPosition();
    
    std::vector<AngledEdge> edges{
        AngledEdge(p0, endpoint0->getAngle(), 0),
        AngledEdge(p1, endpoint1->getAngle(), 1)
    };
    
    auto coordinates = LineStateCoordinates(edges, 1);
    auto state = new LineState(node->getStats(), coordinates);
    getSegment()->addState(state, true);
    
    if (addToModel) {
        state->addToModel();
    }
}

void Line::moveToEndpoints() {
    auto endpoints = getEndpoints();
    auto segments = getSegments();
    
    if (segments.size() == 1) {
        segments[0]->setPositionsOneState({
            endpoints[0]->getPosition(),
            endpoints[1]->getPosition()
        });
    } else {
        // debugger;
        ms::alert("Moving two segments is untested.");
        // segments[0]->setPositions(endpoints[0]->getPosition(), true);
        // segments[1]->setPositions(endpoints[1]->getPosition(), false);
    }
}

bool Line::findIntersections() {
    bool found = false;
    for (auto* segment : getSegments()) {
        if (segment->findIntersections()) {
            found = true;
        }
    }
    if (found) {
        setCost(1e6);
    }
    return found;
}

float Line::getAngle() const {
    return edgeType->getAngle();
}

Vec2 Line::getDir() const {
    return Vec2::unitVec(getAngle());
}

std::vector<Face*> Line::getFaceData() const {
    return {};
}

void Line::draw(View* view, const Vec2& offset) {
    auto endpoints = getEndpoints();
    if (endpoints[0] && endpoints[1]) {
        auto start = endpoints[0]->getPosition().scale(View3D::GENERATED_SCALE);
        auto end = endpoints[1]->getPosition().scale(View3D::GENERATED_SCALE);
        view->drawEdge(start, end, "#000", 0, RENDER_WIDTH_3D);
    }
}

void Line::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    if (auto* view3d = dynamic_cast<View3D*>(context)) {
        auto endpoints = getEndpoints();
        if (endpoints[0]) endpoints[0]->highlight(context, convertToScreen);
        if (endpoints[1]) endpoints[1]->highlight(context, convertToScreen);
        return;
    }
    
    for (auto* segment : getSegments()) {
        segment->highlight(context, convertToScreen);
    }
}

void Line::print() const {
    ms::highlight(this);
}

Line* Line::createFromPositions(const std::vector<Vec2>& positions, float angle0, 
                              EdgeType* edgeType, Stats* stats) {
    float angle1 = Util::fixAngle(angle0 + M_PI);
    Vec2 dir0 = Vec2::unitVec(angle0);
    Vec2 dir1 = Vec2::unitVec(angle1);
    
    auto endpoint0 = new Endpoint(stats, true, edgeType, angle0, dir0, 1, false);
    auto endpoint1 = new Endpoint(stats, false, edgeType, angle1, dir1, 1, false);

    auto vertex0 = new Vertex(stats, positions[0]);
    auto vertex1 = new Vertex(stats, positions[1]);
    vertex0->getNode()->connect(endpoint0);
    vertex1->getNode()->connect(endpoint1);
    
    return createFromEndpoints(stats, {endpoint0, endpoint1});
}

Line* Line::createFromEndpoints(Stats* stats, const std::vector<Endpoint*>& endpoints) {
    auto edgeType = endpoints[0]->getEdgeType();
    auto line = new Line(stats, edgeType);
    
    for (size_t i = 0; i < 2; i++) {
        line->addEndpoint(endpoints[i], i);
    }
    
    line->addSegments({new LineSegment(stats)});
    line->fillFromEndpoints();
    return line;
}

VertexType* Line::getVertexType(EdgeType* edgeType, Shape3D* shape) {
    if (!splitVertexTypes[edgeType->getId()] ||
        splitVertexTypes[edgeType->getId()]->getConnections()[0].edge != edgeType) {
        auto vertexType = new VertexType();
        std::vector<int> faceIds;  // Unsure about this
        vertexType->addEdge(edgeType, true, edgeType->getAngle(), faceIds);
        vertexType->addEdge(edgeType, false, edgeType->getAngle(), faceIds);
        vertexType->setSpliced(true);
        splitVertexTypes[edgeType->getId()] = vertexType;
        if (shape) {
            shape->addVertexType(vertexType);  // Assuming Shape3D has this method
        }
    }
    return splitVertexTypes[edgeType->getId()];
}

Line::SplitResult Line::fullSplit(float s) {
    auto middlePos = Vec2::lerp(
        getEndpoints()[0]->getPosition(),
        getEndpoints()[1]->getPosition(), s);
    
    auto split = this->split();
    auto vertexType = getVertexType(getEdgeType());
    auto newVertex = Vertex::createWithState(node->getStats(), middlePos, 0, 1, vertexType);
    auto addedLines = newVertex->getEndpoints() | 
                     std::views::transform([](auto* ep) { return ep->getLine(); });
    auto addedFaces = newVertex->getEndpoints() |
                     std::views::transform([](auto* ep) { return ep->getFace(); });
    
    // Not sure why this is sometimes different. Some sort of parity.
    int p = !split.lines[0]->getEndpoints()[0] ? 0 : 1;
    
    bool start0 = newVertex->getEndpoints()[0]->getIsAtStart();
    auto endpointT = newVertex->getEndpoints()[start0 ? 0 : 1];
    auto endpointF = newVertex->getEndpoints()[start0 ? 1 : 0];
    
    if (p == 0) {
        split.lines[0]->addEndpoint(endpointF, 0);
        split.lines[1]->addEndpoint(endpointT, 1);
        auto prevEndpoint0 = split.lines[0]->getEndpoints()[1];
        auto prevEndpoint1 = split.lines[1]->getEndpoints()[0];
        bool startPrev0 = prevEndpoint0->getIsAtStart();
        prevEndpoint0->getFace()->insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
        prevEndpoint1->getFace()->insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
        prevEndpoint0->maybeMergeNextFace();
        prevEndpoint1->maybeMergeNextFace();
    } else {
        split.lines[0]->addEndpoint(endpointF, 1);
        split.lines[1]->addEndpoint(endpointT, 0);
        auto prevEndpoint0 = split.lines[0]->getEndpoints()[0];
        auto prevEndpoint1 = split.lines[1]->getEndpoints()[1];
        bool startPrev0 = prevEndpoint0->getIsAtStart();
        prevEndpoint0->getFace()->insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
        prevEndpoint1->getFace()->insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
        prevEndpoint0->maybeMergeNextFace();
        prevEndpoint1->maybeMergeNextFace();
    }
    
    split.lines[0]->fillFromEndpoints(true);
    split.lines[1]->fillFromEndpoints(true);
    
    for (auto* line : addedLines) {
        line->getNode()->destroy();
    }
    for (auto* face : addedFaces) {
        face->getNode()->destroy();
    }
    
    return {split.lines, {newVertex}};
}

} // namespace ms 