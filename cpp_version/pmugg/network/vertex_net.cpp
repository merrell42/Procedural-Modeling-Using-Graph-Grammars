#include "vertex_net.h"
#include "network.h"
#include "connector_group.h"
// #include "view.h"
#include "half_edge_net.h"
#include "../util/util.h"

namespace ms {

int VertexNet::nextId = 0;

VertexNet::VertexNet()
    : network(nullptr)
    // primal(nullptr)
    , group(nullptr)
    , id(nextId++) {}

//void VertexNet::setPrimal(PrimalVertex* p) {
//    primal = p;
//}

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
    
    // Copy half-edges
    halfEdges.clear();
    auto copyHalfEdges = copy->getHalfEdges();
    halfEdges.reserve(copyHalfEdges.size());
    
    for (auto* halfEdge : copyHalfEdges) {
        halfEdges.push_back(halfEdge ? network->convertHalfEdge(copyNet, halfEdge) : nullptr);
    }
    
    // Copy group
    //auto* copyGroup = copy->getGroup();
    //group = copyGroup ? network->convertConnectorGroup(copyNet, copyGroup) : nullptr;
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

//void VertexNet::highlight(View* view, const DrawOptions& options) {
//    auto highlightOptions = HalfEdgeSet::create(halfEdges);
//    network->highlight(view, highlightOptions);
//}
//
//void VertexNet::print() const {
//    auto* interior = primal->getInterior();
//    if (interior != this) {
//        std::cout << "Boundary Vertex" << std::endl;
//        auto* halfEdge = interior->getHalfEdges()[0];
//        if (halfEdge->getEdge()) {
//            halfEdge->getEdge()->print();
//        } else {
//            halfEdge->getPrev()->getEdge()->print();
//        }
//    } else {
//        ms::highlight(this);
//    }
//}
//
//Json VertexNet::export() const {
//    Json json;
//    
//    // Export half-edges
//    json["halfEdges"] = Json::array();
//    for (auto* halfEdge : halfEdges) {
//        json["halfEdges"].push_back(halfEdge ? network->halfEdgeIndex(halfEdge) : -1);
//    }
//    
//    // Export group
//    json["group"] = group ? network->connectorGroupIndex(group) : -1;
//    
//    return json;
//}

void VertexNet::import(const Json& json) {
    // Import half-edges
    halfEdges.clear();
    auto& networkHalfEdges = network->getHalfEdges();
    
    if (json.contains("halfEdges")) {
        for (const auto& index : json["halfEdges"]) {
            int idx = index.get<int>();
            halfEdges.push_back(idx >= 0 ? networkHalfEdges[idx] : nullptr);
        }
    }
    
    // Import group
    //int groupIndex = json["group"].get<int>();
    //group = groupIndex >= 0 ? network->getConnectorGroups()[groupIndex] : nullptr;
}

} // namespace ms 