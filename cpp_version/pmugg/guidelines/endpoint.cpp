#include "endpoint.h"
#include "global_settings.h"
#include <cmath>

namespace ms {

Endpoint::Endpoint(Stats* stats, bool isAtStart, EdgeType* edgeType, float angle, 
                  Vec2* dir, float scale, bool createFace, int faceIndex)
    : isAtStart(isAtStart)
    , edgeType(edgeType)
    , dir(dir)
    , scale(scale)
    , faceIndex(faceIndex)
    , faceTypeCached(nullptr)
{
    auto properties = std::map<std::string, Property*>{
        {"face", new SingleProperty("face")},
        {"line", new SingleProperty("line", true)},
        {"vertex", new SingleProperty("vertex")},
        {"vertexState", new SingleProperty("vertexState")},
        {"angle", new ValueProperty("angle")},
        {"faceConnection", new AlternativeArray("faceConnection", false)}
    };

    node = new Node(this, stats, "endpoint", properties);
    node->setValue("angle", angle);

    if (createFace) {
        auto faceType = GlobalSettings::get("Use Network") ? this->faceType() : nullptr;
        auto face = new Face(faceType, stats);
        face->createGroup();
        face->getNode()->connect(this);
    }
}

Endpoint::~Endpoint() {
    delete node;
}

Node* Endpoint::getNode() const {
    return node;
}

bool Endpoint::isRigid() const {
    return edgeType->getIsRigid();
}

Line* Endpoint::getLine() const {
    return node->get<Line*>("line");
}

Face* Endpoint::getFace() const {
    return node->get<Face*>("face");
}

float Endpoint::getAngle() const {
    return node->get<float>("angle");
}

void Endpoint::setAngle(float angle) {
    node->setValue("angle", angle);
}

Vec2 Endpoint::getDir() const {
    return *dir;
}

std::vector<FaceConnection*> Endpoint::getConnections() const {
    return node->get<std::vector<FaceConnection*>>("faceConnection");
}

bool Endpoint::getIsAtStart() const {
    return isAtStart;
}

EdgeType* Endpoint::getEdgeType() const {
    return edgeType;
}

Vertex* Endpoint::getVertex() const {
    return node->get<Vertex*>("vertex");
}

VertexState* Endpoint::getVertexState() const {
    return node->get<VertexState*>("vertexState");
}

bool Endpoint::isConflicted() const {
    return !getTwin();
}

Vec2 Endpoint::getPosition() const {
    return getVertex()->getPosition();
}

LineState* Endpoint::getLineState() const {
    return getLine() ? getLine()->getState(isAtStart) : nullptr;
}

Segment* Endpoint::getSegment() const {
    return getLine()->getSegment(isAtStart);
}

void Endpoint::move(const Vec2& newPosition) {
    TimerG::start("endpoint move");
    if (GlobalSettings::get("Full Move")) {
        getSegment()->setPosition(newPosition, isAtStart);
    } else {
        getLineState()->setPosition(newPosition, isAtStart);
    }
    TimerG::stop("endpoint move");
}

Endpoint* Endpoint::getTwin() const {
    auto line = getLine();
    if (line) {
        auto endpoints = line->getEndpoints();
        auto index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();
        return endpoints[1 - index];
    }
    return nullptr;
}

float Endpoint::desiredLength() const {
    return scale * edgeType->getEdgeLength();
}

Vec2 Endpoint::idealOffset() const {
    Vec2 offset = Vec2::unitVec(getAngle());
    offset.scale(desiredLength());
    return offset;
}

Vec2 Endpoint::randomOffset() const {
    Vec2 u = idealOffset();
    float deviationAmount = DEVIATION * desiredLength();
    Vec2 deviation(deviationAmount * randomGaussian(), deviationAmount * randomGaussian());
    u.add(deviation);
    return u;
}

FaceType* Endpoint::faceType() {
    if (!faceTypeCached) {
        auto faceData = edgeType->getFaceData();
        if (GlobalSettings::get("Use Network")) {
            return faceData[faceIndex].type;
        } else {
            throw std::runtime_error("This should only happen for networks.");
        }
    }
    return faceTypeCached;
}

Endpoint* Endpoint::next3D() {
    return nextOnFace(faceType());
}

Endpoint* Endpoint::next() {
    if (GlobalSettings::get("Use Network")) {
        auto endpoints = getFace()->getEndpoints();
        auto N = endpoints.size();
        auto index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();
        return endpoints[(index + 1) % N];
    }

    if (edgeType->is3D()) {
        return next3D();
    }
    auto twin = getTwin();
    return twin ? twin->clockwise() : nullptr;
}

Endpoint* Endpoint::prev() {
    if (GlobalSettings::get("Use Network")) {
        auto endpoints = getFace()->getEndpoints();
        auto index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();
        if (index == 0) {
            auto N = endpoints.size();
            index = N;
        }
        return endpoints[index - 1];
    }

    auto counter = this->counter();
    return counter ? counter->getTwin() : nullptr;
}

Endpoint* Endpoint::clockwise() {
    auto vertexState = getVertexState();
    if (!vertexState) {
        return nullptr;
    }
    auto endpoints = vertexState->getEndpoints();
    auto index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();
    if (index == -1) {
        throw std::runtime_error("Vertex is missing endpoint in clockwise.");
    }
    auto nextIndex = index > 0 ? index - 1 : endpoints.size() - 1;
    return endpoints[nextIndex];
}

Endpoint* Endpoint::counter() {
    auto vertexState = getVertexState();
    if (!vertexState) {
        return nullptr;
    }
    auto endpoints = vertexState->getEndpoints();
    auto index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();
    if (index == -1) {
        throw std::runtime_error("Vertex is missing endpoint in next.");
    }
    auto prevIndex = (index + 1) % endpoints.size();
    return endpoints[prevIndex];
}

Face* Endpoint::nextFace() {
    auto lineStates = getVertex()->getLineStates();
    if (!lineStates.empty()) {
        return lineStates[0]->getLine()->getEndpoints()[0]->getFace();
    } else {
        next()->print();
        return nullptr;
    }
}

float Endpoint::angleOffset() {
    auto defaultAngle = getLine()->getEdgeType()->getAngle() + (getIsAtStart() ? 0 : M_PI);
    return Util::fixAngle(getAngle() - defaultAngle);
}

void Endpoint::mergeFaces(Endpoint* next) {
    getFace()->append(next->getFace());
}

void Endpoint::maybeMergeNextFace() {
    auto next = this->next();
    if (next) {
        getFace()->append(next->getFace());
    }
}

void Endpoint::maybeMergePrevFace() {
    auto prev = this->prev();
    if (prev) {
        prev->getFace()->append(getFace());
    }
}

void Endpoint::transfer(Endpoint* replacement) {
    auto index = getLine()->getEndpoints()[0] == this ? 0 : 1;
    replacement->addEndpoint(this, index);
}

void Endpoint::checkSupport() {
    if (node->isDestroyed()) {
        return;
    }
    if (!getVertexState() && (!getTwin() || !getTwin()->getVertexState())) {
        getLine()->destroy();
    }
}

bool Endpoint::hasRightBound() {
    auto connections = getConnections();
    return !connections.empty() && !connections[0]->isLeft(this);
}

int Endpoint::spliceIndex(FaceConnection* connectionA) {
    auto connections = getConnections();
    int index = 0;
    
    if (hasRightBound()) {
        index++;
    }
    
    float yA = connectionA->getCoordinates()[0].y;
    float yB = std::numeric_limits<float>::infinity();
    
    while (yA < yB) {
        if (index >= connections.size()) {
            return index;
        }
        yB = connections[index]->getCoordinates()[0].y;
        index++;
    }
    return index - 1;
}

} // namespace ms