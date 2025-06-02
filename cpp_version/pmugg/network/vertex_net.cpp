#include "pch.h"
#include "vertex_net.h"
#include "network.h"
#include "connector_group.h"
#include "half_edge_net.h"
#include "../util/util.h"

namespace ms {

int VertexNet::nextId = 0;

VertexNet::VertexNet()
    : network(nullptr)
    , type(nullptr)
    , id(nextId++) {}

VertexNet* VertexNet::connectNet(Network* net) {
    network = net;
    network->addVertex(this);
    return this;
}

void VertexNet::setHalfEdge(HalfEdgeNet* halfEdge, int index) {
    if (index >= halfEdges.size()) {
        halfEdges.resize(index + 1, nullptr);
    }
    halfEdges[index] = halfEdge;
}

int VertexNet::connectorIndex() const {
    auto bVertices = network->getBVertices();
    auto it = std::find(bVertices.begin(), bVertices.end(), this);
    if (it != bVertices.end()) {
        // Compute the index
        return std::distance(bVertices.begin(), it);
    } else {
        return -1;
    }
}

void VertexNet::copyConnection(const VertexNet* copy) {
    auto* copyNet = copy->getNetwork();

    halfEdges.clear();
    auto copyHalfEdges = copy->getHalfEdges();
    halfEdges.reserve(copyHalfEdges.size());
    
    for (auto* halfEdge : copyHalfEdges) {
        halfEdges.push_back(halfEdge ? network->convertHalfEdge(copyNet, halfEdge) : nullptr);
    }
}

bool VertexNet::inNetwork() const {
    return network && std::find(network->getVertices().begin(),
                              network->getVertices().end(),
                              this) != network->getVertices().end();
}

EdgeNet* VertexNet::interiorEdge() const {
    for (auto* halfEdge : halfEdges) {
        if (halfEdge) {
            auto edge = halfEdge->getEdge();
            if (edge) {
                return edge;
            }
        }
    }
    return nullptr;
}

void VertexNet::import(const Json& json) {
    halfEdges.clear();
    auto& networkHalfEdges = network->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& index : json["halfEdges"]) {
            int idx = index.get<int>();
            halfEdges.push_back(idx >= 0 ? networkHalfEdges[idx] : nullptr);
        }
    }
}

} // namespace ms 