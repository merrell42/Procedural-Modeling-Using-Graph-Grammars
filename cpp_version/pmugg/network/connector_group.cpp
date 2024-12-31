#include "connector_group.h"
#include "network.h"
#include "vertex.h"
#include "util.h"

namespace ms {

int ConnectorGroup::nextId = 0;

ConnectorGroup::ConnectorGroup()
    : network(nullptr)
    , id(nextId++) {}

Network* ConnectorGroup::getNetwork() const {
    return network;
}

const std::vector<Vertex*>& ConnectorGroup::getConnectors() const {
    return connectors;
}

int ConnectorGroup::getId() const {
    return id;
}

ConnectorGroup* ConnectorGroup::connectNet(Network* net) {
    network = net;
    network->addConnectorGroup(this);
    return this;
}

void ConnectorGroup::addConnector(Vertex* connector, int index) {
    if (index >= connectors.size()) {
        connectors.resize(index + 1, nullptr);
    }
    connectors[index] = connector;
    if (connector) {
        connector->setGroup(this);
    }
}

void ConnectorGroup::copyConnection(const ConnectorGroup* copy) {
    auto* copyNet = copy->getNetwork();
    auto copyConnectors = copy->getConnectors();
    
    connectors.clear();
    connectors.reserve(copyConnectors.size());
    
    for (auto* vertex : copyConnectors) {
        if (vertex) {
            connectors.push_back(network->convertVertex(copyNet, vertex));
        } else {
            connectors.push_back(nullptr);
        }
    }
}

Json ConnectorGroup::export() const {
    Json json;
    json["connectors"] = Json::array();
    
    for (auto* vertex : connectors) {
        if (vertex) {
            json["connectors"].push_back(network->vertexIndex(vertex));
        } else {
            json["connectors"].push_back(nullptr);
        }
    }
    
    return json;
}

void ConnectorGroup::import(const Json& json) {
    connectors.clear();
    auto& vertices = network->getVertices();
    
    for (const auto& index : json["connectors"]) {
        if (index.is_null()) {
            connectors.push_back(nullptr);
        } else {
            connectors.push_back(vertices[index.get<int>()]);
        }
    }
}

} // namespace ms 