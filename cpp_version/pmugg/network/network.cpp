#include "network.h"
#include "../shape/edge.h"
#include "../network/half_edge_net.h"
#include "../network/vertex_net.h"
#include "../network/edge_net.h"
#include "../network/face_net.h"
#include "../network/bound_net.h"
#include "../graph_drawing/face.h"
// #include "connector_group.h"
// #include "bound_net.h"
// #include "view.h"
#include "../util/util.h"
// #include "graph.h"
#include <algorithm>

namespace ms {

int Network::nextId = 0;

Network::Network()
    : id(nextId++)
    // , boundNet(nullptr)
{}

void Network::addVertex(VertexNet* vertex) {
    vertices.push_back(vertex);
}

void Network::addEdge(EdgeNet* edge) {
    edges.push_back(edge);
}

void Network::addHalfEdge(HalfEdgeNet* halfEdge) {
    halfEdges.push_back(halfEdge);
}

void Network::addFace(FaceNet* face) {
    faces.push_back(face);
}

//void Network::addConnectorGroup(ConnectorGroup* group) {
//    connectorGroups.push_back(group);
//}

//void Network::setBoundNet(BoundNet* net) {
//    boundNet = net;
//}

void Network::removeVertex(VertexNet* vertex) {
    vertices.erase(std::remove(vertices.begin(), vertices.end(), vertex), vertices.end());
}

void Network::removeEdge(EdgeNet* edge) {
    edges.erase(std::remove(edges.begin(), edges.end(), edge), edges.end());
}

void Network::removeHalfEdge(HalfEdgeNet* halfEdge) {
    halfEdges.erase(std::remove(halfEdges.begin(), halfEdges.end(), halfEdge), halfEdges.end());
}

void Network::removeFace(FaceNet* face) {
    faces.erase(std::remove(faces.begin(), faces.end(), face), faces.end());
}

//void Network::removeConnectorGroup(ConnectorGroup* group) {
//    Util::remove(group, connectorGroups);
//}

VertexNet* Network::convertVertex(Network* networkB, VertexNet* vertexB) {
    return vertexB ? vertices[networkB->vertexIndex(vertexB)] : nullptr;
}

EdgeNet* Network::convertEdge(Network* networkB, EdgeNet* edgeB) {
    return edgeB ? edges[networkB->edgeIndex(edgeB)] : nullptr;
}

HalfEdgeNet* Network::convertHalfEdge(Network* networkB, HalfEdgeNet* halfEdgeB) {
    return halfEdgeB ? halfEdges[networkB->halfEdgeIndex(halfEdgeB)] : nullptr;
}

FaceNet* Network::convertFace(Network* networkB, FaceNet* faceB) {
    return faceB ? faces[networkB->faceIndex(faceB)] : nullptr;
}

//ConnectorGroup* Network::convertConnectorGroup(Network* networkB, ConnectorGroup* groupB) {
//    return groupB ? connectorGroups[networkB->connectorGroupIndex(groupB)] : nullptr;
//}

int Network::vertexIndex(VertexNet* vertex) const {
    return vertex ? (int)(std::find(vertices.begin(), vertices.end(), vertex) - vertices.begin()) : -1;
}

int Network::edgeIndex(EdgeNet* edge) const {
    return edge ? (int)(std::find(edges.begin(), edges.end(), edge) - edges.begin()) : -1;
}

int Network::halfEdgeIndex(HalfEdgeNet* halfEdge) const {
    return halfEdge ? (int)(std::find(halfEdges.begin(), halfEdges.end(), halfEdge) - halfEdges.begin()) : -1;
}

int Network::faceIndex(FaceNet* face) const {
    return face ? (int)(std::find(faces.begin(), faces.end(), face) - faces.begin()) : -1;
}

//int Network::connectorGroupIndex(ConnectorGroup* group) const {
//    return group ? (int)(std::find(connectorGroups.begin(), connectorGroups.end(), group) - connectorGroups.begin()) : -1;
//}

//bool Network::isBoundary() const {
//    return boundNet && boundNet->getBoundary() == this;
//}
//
//bool Network::isInterior() const {
//    return boundNet && boundNet->getInterior() == this;
//}

//void Network::draw(View* view, const DrawOptions& options) {
//    if (vertices.empty()) {
//        drawEmptyNetwork(view, options);
//    } else {
//        drawNetworkElements(view, options);
//    }
//}
//
//void Network::highlight(View* view, const DrawOptions& options) {
//    auto size = Graph::HIGHLIGHTED_SIZE;
//    auto offset = options.offset.value_or(Vec2(20, view->getCanvas().height - size - 20));
//    
//    DrawOptions newOptions = options;
//    newOptions.rect = {offset.x, offset.x + size, offset.y, offset.y + size, 0, 100};
//    draw(view, newOptions);
//}
//
//void Network::print() const {
//    ms::highlight(this);
//}

//Network* Network::copy() const {
//    return Network::import(export());
//}

Network* Network::import(const Json & json, Shape3D* shape) {
    auto interior = json["interior"];

    auto result = new Network();

    for (const auto& vertex : interior["vertices"]) {
        (new VertexNet())->connectNet(result);
    }
    for (const auto& edge : interior["edges"]) {
        (new EdgeNet())->connectNet(result);
    }
    for (const auto& halfEdge : interior["halfEdges"]) {
        (new HalfEdgeNet(true))->connectNet(result);
    }
    for (const auto& face : interior["faces"]) {
        (new FaceNet())->connectNet(result);
    }
    //for (const auto& cGroup : json["connectorGroups"]) {
    //    (new ConnectorGroup())->connectNet(result);
    //}

    for (size_t index = 0; index < interior["vertices"].size(); ++index) {
        result->getVertices()[index]->import(interior["vertices"][index]);
    }
    for (size_t index = 0; index < interior["edges"].size(); ++index) {
        result->getEdges()[index]->import(interior["edges"][index]);
    }
    for (size_t index = 0; index < interior["halfEdges"].size(); ++index) {
        result->getHalfEdges()[index]->import(interior["halfEdges"][index]);
    }
    for (size_t index = 0; index < interior["faces"].size(); ++index) {
        result->getFaces()[index]->import(interior["faces"][index]);
    }
    //for (size_t index = 0; index < json["connectorGroups"].size(); ++index) {
    //    result->getConnectorGroups()[index]->import(json["connectorGroups"][index]);
    //}

    // Get the types.
    for (size_t index = 0; index < json["vertices"].size(); ++index) {
        auto vertexData = json["vertices"][index];
        int type = vertexData["type"].get<int>();
        result->getVertices()[index]->setType(shape->vertexTypes[type]);

        result->getVertices()[index]->kind = vertexData["kind"].get<std::string>();
    }
    for (size_t index = 0; index < json["edges"].size(); ++index) {
        auto edgeData = json["edges"][index];
        int type = edgeData["type"].get<int>();
        result->getEdges()[index]->setType(shape->edgeTypes[type]);
    }
    if (json.contains("morphism")) {
        auto morphism = json["morphism"];
        for (size_t index = 0; index < morphism["halfs"].size(); ++index) {
            int hIndex = morphism["halfs"][index].get<int>();
            auto bHalf = hIndex >= 0 ? result->getHalfEdges()[hIndex] : nullptr;
            result->bHalfEdges.push_back(bHalf);
        }
        for (size_t index = 0; index < morphism["vertices"].size(); ++index) {
            int hIndex = morphism["vertices"][index].get<int>();
            auto bVertex = hIndex >= 0 ? result->getVertices()[hIndex] : nullptr;
            result->bVertices.push_back(bVertex);
        }
    }

    return result;
}

// Remove any spliced edges.
void Network::removeSplices() {
    // Check if any half edges are spliced
    bool hasSplices = std::any_of(
        getHalfEdges().begin(),
        getHalfEdges().end(),
        [](HalfEdgeNet* half) { return half->isSpliced(); }
    );

    if (!hasSplices) {
        return;
    }

    // This part is dangerous in Javascript.
    // result->getConnectors();

    auto halfEdges = getHalfEdges(); // Make a copy of the vector

    // First pass: merge edges
    for (auto* half : halfEdges) {
        auto* next = half->getNext();
        if (!half->isSpliced() && next && next->isSpliced()) {
            auto* newNext = next->getTwin()->getNext();
            half->getEdge()->merge(newNext->getEdge(), half->getForward());
        }
    }

    // Second pass: remove spliced edges
    halfEdges = getHalfEdges(); // Get fresh copy after merges
    for (auto* half : halfEdges) {
        if (half->isSpliced()) {
            removeHalfEdge(half);
            auto* vertex = half->getVertex();
            if (vertex->inNetwork()) {
                removeVertex(vertex);
            }
            auto* edge = half->getEdge();
            if (edge->inNetwork()) {
                removeEdge(edge);
            }
        }
    }
}

//Vec3 Network::combineTangents(const Vec3& u, const Vec3& v) {
//    if (u.dot(v) > 0.9f) {
//        return u;
//    }
//    
//    float uv = u.dot(v);
//    Matrix2f A({
//        {1, uv},
//        {uv, 1}
//    });
//    auto Ainv = A.inverse();
//    auto B = Ainv * Vector2f(1, 1);
//    
//    return u.scale(B.x()) + v.scale(B.y());
//}

} // namespace ms 