#include "vertex_state.h"
#include "vertex.h"
#include "endpoint.h"
#include "ring_instance.h"
#include "vertex_type.h"
#include "line.h"
#include "line_segment.h"
#include "settings.h"
#include "util.h"
#include <algorithm>

namespace ms {

VertexState::VertexState(Stats* stats, Vertex* vertex, float angle, float scale, 
                        VertexType* type, bool primal)
    : angle(angle)
    , scale(scale)
    , type(type) {
    
    auto connections = type->getConnections();
    std::map<std::string, std::unique_ptr<Property>> properties;
    properties["endpoint"] = std::make_unique<RequiredArray>("endpoint", true, connections.size());
    properties["ringInstance"] = std::make_unique<AlternativeArray>("ringInstance", false);
    properties["vertex"] = std::make_unique<SingleProperty>("vertex");
    properties["cost"] = std::make_unique<ValueProperty>("cost", "valence");

    node = std::make_unique<Node>(this, stats, "vertexState", std::move(properties));
    node->setValue("cost", type->getConnections().size() - 2);
    node->connect(vertex);
}

Node* VertexState::getNode() const {
    return node.get();
}

Vertex* VertexState::getVertex() const {
    return node->get("vertex");
}

std::vector<Endpoint*> VertexState::getEndpoints() const {
    return node->get("endpoint");
}

Endpoint* VertexState::getEndpoint(int index) const {
    auto endpoints = getEndpoints();
    return index < endpoints.size() ? endpoints[index] : nullptr;
}

std::vector<RingInstance*> VertexState::getRingInstances() const {
    return node->get("ringInstance");
}

VertexType* VertexState::getType() const {
    return type;
}

float VertexState::getAngle() const {
    return angle;
}

float VertexState::getScale() const {
    return scale;
}

void VertexState::resolveEndpoints() {
    if (globalSettings.getBool("Use Network")) {
        resolveEndpointsNet();
    } else {
        resolveEndpointsGraph();
    }
}

void VertexState::resolveEndpointsNet() {
    auto connections = type->getConnections();
    int vertexIndex = 0;
    
    for (size_t i = 0; i < connections.size(); i++) {
        const auto& connection = connections[i];
        auto edgeType = connection.edge;
        auto faceData = edgeType->getFaceData();
        
        for (size_t faceIndex = 0; faceIndex < faceData.size(); faceIndex++) {
            auto faceDatum = faceData[faceIndex];
            bool position = faceDatum.onRight ^ connection.isAtStart;
            
            if (position) {
                createEndpointNet(connection, vertexIndex, faceIndex);
                vertexIndex++;
            }
        }
    }
}

void VertexState::resolveEndpointsGraph() {
    for (size_t i = 0; i < getEndpoints().size(); i++) {
        if (!getEndpoint(i)) {
            createEndpointGraph(i);
        }
    }
}

void VertexState::remove() {
    auto ringInstances = getRingInstances();
    for (auto* instance : ringInstances) {
        node->disconnect(instance);
    }
}

void VertexState::removeLeaves() {
    auto ringInstances = getRingInstances();
    for (auto* instance : ringInstances) {
        node->disconnect(instance);
    }
}

int VertexState::endpointIndex(Endpoint* endpoint) const {
    auto endpoints = getEndpoints();
    return std::find(endpoints.begin(), endpoints.end(), endpoint) - endpoints.begin();
}

void VertexState::addEndpoint(Endpoint* endpoint, int index) {
    auto oldState = endpoint->getVertexState();
    if (oldState) {
        endpoint->getNode()->disconnect(oldState);
    }

    node->insert(endpoint, index);
    endpoint->getNode()->connect(getVertex());
    endpoint->maybeMergePrevFace();

    if (getEndpoints().size() > 1) {
        auto twin = endpoint->getTwin();
        if (twin) {
            twin->maybeMergeNextFace(twin);
        }
    }
}

Endpoint* VertexState::createEndpointNet(const Connection& connection, int vertexIndex, int faceIndex) {
    if (node->isDestroyed()) {
        ms::alert("Error in createEndpoint.");
        return nullptr;
    }

    auto dir = connection.dir.copy();
    if (angle != 0) {
        dir.rotate(angle);
    }

    auto endpointAngle = ms::util::fixAngle(angle + connection.angle);
    auto stats = node->getStats();
    auto endpoint = new Endpoint(stats, connection.isAtStart, connection.edge, 
                               endpointAngle, dir, scale, true, faceIndex);

    auto line = new Line(stats, connection.edge);
    auto segment = new LineSegment(stats);
    line->addSegments({segment});
    line->addEndpoint(endpoint, faceIndex);
    addEndpoint(endpoint, vertexIndex);

    return endpoint;
}

Endpoint* VertexState::createEndpointGraph(int index) {
    if (node->isDestroyed()) {
        ms::alert("Error in createEndpoint.");
        return nullptr;
    }

    auto connections = type->getConnections();
    const auto& connection = connections[index];
    
    auto dir = connection.dir.copy();
    if (angle != 0) {
        dir.rotate(angle);
    }

    auto endpointAngle = ms::util::fixAngle(angle + connection.angle);
    auto stats = node->getStats();
    auto endpoint = new Endpoint(stats, connection.isAtStart, connection.edge, 
                               endpointAngle, dir, scale, true);

    auto line = new Line(stats, connection.edge);
    auto segment = new LineSegment(stats);
    line->addSegments({segment});
    line->addEndpoint(endpoint, connection.isAtStart ? 0 : 1);
    addEndpoint(endpoint, index);

    return endpoint;
}

void VertexState::highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen) {
    for (auto* endpoint : getEndpoints()) {
        if (endpoint) {
            endpoint->highlight(context, convertToScreen);
        }
    }
}

void VertexState::print() const {
    auto endpoints = getEndpoints();
    for (auto* endpoint : endpoints) {
        if (endpoint) {
            std::cout << endpoint->getAngle() * 180 / M_PI << std::endl;
        }
    }
    std::cout << getVertex()->getPosition().toString() << std::endl;
    ms::highlight(this);
}

} // namespace ms 