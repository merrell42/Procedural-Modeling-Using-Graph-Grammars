#include "vertex.h"
#include "vertex_state.h"
#include "../guidelines/endpoint.h"
#include "line_state.h"
#include "line.h"
#include "face.h"
#include "../grid/cell.h"
// #include "decoration_view.h"
#include "../util/util.h"
#include "../node/node.h"
#include <algorithm>

namespace ms {

bool Vertex::vertexMergingEnabled = false;

Vertex::Vertex(Stats* stats, const Vec2& position)
    // : tileSeeds{Util::random(100), Util::random(100)} ,
    // imageSeed(Util::random(100)) {
    
    std::map<std::string, std::unique_ptr<Property>> properties;
    properties["collection"] = std::make_unique<SingleProperty>("collection");
    properties["endpoint"] = std::make_unique<AlternativeArray>("endpoint", false, false);
    properties["lineState"] = std::make_unique<RequiredArray>("lineState", false, 2);
    properties["position"] = std::make_unique<ValueProperty>("position");
    properties["vertexState"] = std::make_unique<SingleProperty>("vertexState");
    properties["face"] = std::make_unique<SingleProperty>("face");

    node = std::make_unique<Node>(this, stats, "vertex", std::move(properties));
    node->setChangeHandler("vertexState", [this]() { updateStats(); });
    node->setChangeHandler("endpoint", [this]() { onEndpointsChanged(); });
    node->setDestroyHandler([this]() { onDestroy(); });
    node->setValue("position", position);
}

Node* Vertex::getNode() const {
    return node.get();
}

std::vector<Endpoint*> Vertex::getEndpoints() const {
    return node->get("endpoint");
}

Endpoint* Vertex::getEndpoint(int index) const {
    auto endpoints = getEndpoints();
    return index < endpoints.size() ? endpoints[index] : nullptr;
}

VertexState* Vertex::getState() const {
    return node->get("vertexState");
}

std::vector<LineState*> Vertex::getLineStates() const {
    return node->get("lineState");
}

Vec2 Vertex::getPosition() const {
    return node->get("position");
}

int Vertex::getId() const {
    return node->getId();
}

void Vertex::addLineState(LineState* lineState, int index) {
    node->doubleInsert(1 - index, index, lineState);
}

void Vertex::removeLineStates() {
    auto lineStates = getLineStates();
    if (lineStates[0]) node->disconnect(lineStates[0]);
    if (lineStates[1]) node->disconnect(lineStates[1]);
}

void Vertex::transferLineStates(Vertex* vertexB) {
    auto lineStates = vertexB->getLineStates();
    for (int i = 0; i < 2; i++) {
        if (lineStates[i]) {
            vertexB->getNode()->disconnect(lineStates[i]);
            addLineState(lineStates[i], i);
        }
    }
}

void Vertex::resolveEndpoints() {
    auto state = getState();
    if (state) {
        state->resolveEndpoints();
    }
}

void Vertex::setPosition(const Vec2& position) {
    node->setValue("position", position);
}

bool Vertex::isMutable() const {
    return node->getStats()->getModel()->cellFromPosition(getPosition())->isMutable();
}

bool Vertex::isMoveable() const {
    if (!isMutable()) {
        return false;
    }
    return !std::any_of(getEndpoints().begin(), getEndpoints().end(),
        [](Endpoint* endpoint) {
            return endpoint->getTwin() && !endpoint->getTwin()->getVertex()->isMutable();
        });
}

void Vertex::merge(Vertex* vertexB, Endpoint* connectingEndpoint) {
    auto endpointsB = vertexB->getEndpoints();
    auto stateB = vertexB->getState();
    
    if (stateB) {
        stateB->remove();
    }

    for (auto* endpointB : endpointsB) {
        if (endpointB != connectingEndpoint) {
            endpointB->getNode()->connect(this);
            endpointB->getLine()->reconstructFromEndpoints();
        }
    }

    if (connectingEndpoint) {
        connectingEndpoint->getNode()->destroy();
    }
    vertexB->getNode()->destroy();

    if (connectingEndpoint) {
        // Remove endpoints that are part of a line connecting the merged vertices
        auto badEndpoints = getEndpoints();
        badEndpoints.erase(
            std::remove_if(badEndpoints.begin(), badEndpoints.end(),
                [](Endpoint* endpoint) {
                    auto endpoints = endpoint->getLine()->getEndpoints();
                    return endpoints[0]->getVertex() == endpoints[1]->getVertex();
                }),
            badEndpoints.end());

        for (auto* endpoint : badEndpoints) {
            endpoint->getNode()->destroy();
        }
    }
}

bool Vertex::hasConflict() const {
    return std::any_of(getEndpoints().begin(), getEndpoints().end(),
        [](Endpoint* endpoint) { return endpoint->isConflicted(); });
}

//void Vertex::fillHighlight(RenderData& renderData) {
//    for (auto* endpoint : getEndpoints()) {
//        endpoint->fillHighlight(renderData);
//    }
//}

void Vertex::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    for (auto* endpoint : getEndpoints()) {
        endpoint->highlight(context, convertToScreen);
    }
}

//void Vertex::fillRenderData(RenderData& renderData) {
//    auto state = getState();
//    if (!state) return;
//
//    auto decoration = state->getType()->getDecoration();
//    if (!decoration || !decoration->hasImage()) {
//        auto endpoints = getEndpoints();
//        if (endpoints.size() >= 2) {
//            for (size_t i = 0; i < endpoints.size(); i++) {
//                auto i1 = (i + 1) % endpoints.size();
//                auto endpoint0 = endpoints[i];
//                auto endpoint1 = endpoints[i1];
//                auto brush0 = endpoint0->getEdgeType()->getBrush();
//                auto brush1 = endpoint1->getEdgeType()->getBrush();
//
//                if (brush0 && brush0 == brush1) {
//                    fillFromEdges(renderData, endpoint0, endpoint1);
//                }
//            }
//        }
//        return;
//    }
//
//    auto imageName = DecorationView::pickImage(decoration->get("Image"), imageSeed);
//    auto& textureData = renderData.getTextureData(imageName);
//    
//    auto centerX = decoration->get("Center X");
//    auto centerY = -decoration->get("Center Y");
//    auto w = decoration->get("Width") / 2;
//    auto h = decoration->get("Height") / 2;
//    
//    auto center = getPosition();
//    center.move(centerX, centerY);
//
//    Util::fastConcat(textureData.texcoords, LineStateCoordinates::DEFAULT_TEXCOORDS);
//    std::vector<float> newPositions = {
//        center.x - w, center.y + h,
//        center.x - w, center.y - h,
//        center.x + w, center.y + h,
//        center.x + w, center.y + h,
//        center.x - w, center.y - h,
//        center.x + w, center.y - h
//    };
//    Util::fastConcat(textureData.positions, newPositions);
//}

void Vertex::draw(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    auto state = getState();
    if (!state) return;

    auto decoration = state->getType()->getDecoration();
    if (!decoration || !decoration->hasImage()) return;

    auto scale = convertToScreen.scale;
    auto pos = convertToScreen(getPosition());
    auto imageName = decoration->get("Image");
    auto centerX = scale * decoration->get("Center X");
    auto centerY = scale * decoration->get("Center Y");
    // auto image = DecorationView::getImage(imageName, imageSeed);
    
    // Drawing implementation would go here depending on graphics system
    // This is a placeholder for the actual drawing code
}

void Vertex::print() const {
    std::cout << "Vertex " << node->getId() << ": " << getPosition().toString() << std::endl;
    for (auto* endpoint : getEndpoints()) {
        std::cout << "  Angle: " << (endpoint->getAngle() * 180 / M_PI) << std::endl;
    }
}

Vertex* Vertex::createWithState(Stats* stats, const Vec2& position, float angle, 
                              float scale, VertexType* type, bool primal) {
    auto vertex = new Vertex(stats, position);
    auto newState = new VertexState(stats, vertex, angle, scale, type, primal);
    newState->resolveEndpoints();
    return vertex;
}

bool Vertex::compare(Endpoint* endpointA, Endpoint* endpointB) {
    return endpointA->getAngle() > endpointB->getAngle();
}

void Vertex::updateStats() {
    if (node->isDestroyed()) {
        node->stats->removeVertex(node.get());
    } else {
        node->stats->addVertex(node.get());
    }
}

void Vertex::onEndpointsChanged() {
    if (getEndpoints().empty()) {
        auto lineStates = getLineStates();
        if (lineStates[0]) {
            lineStates[0]->merge(lineStates[1]);
        }
        node->destroy();
    }
    updateStats();
}

void Vertex::onDestroy() {
    updateStats();
}

} // namespace ms 