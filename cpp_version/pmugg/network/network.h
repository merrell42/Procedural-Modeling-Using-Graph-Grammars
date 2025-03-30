#pragma once
#include <vector>
#include <memory>
#include "../shape/vec3.h"
#include "vertex_net.h"
#include "edge_net.h"
#include "face_net.h"
#include "half_edge_net.h"
#include "bound_net.h"

namespace ms {

class VertexNet;
class EdgeNet;
class HalfEdgeNet;
class FaceNet;
// class ConnectorGroup;
// class BoundNet;
class View;
class Shape3D;
struct DrawOptions;

class Network {
public:
    Network();
    ~Network() = default;

    // Core accessors
    const std::vector<VertexNet*>& getVertices() const { return vertices; }
    const std::vector<EdgeNet*>& getEdges() const { return edges; }
    const std::vector<HalfEdgeNet*>& getHalfEdges() const { return halfEdges; }
    const std::vector<FaceNet*>& getFaces() const { return faces; }
    const std::vector<VertexNet*>& getBVertices() const { return bVertices; }
    const std::vector<HalfEdgeNet*>& getBHalfEdges() const { return bHalfEdges; }
    EdgeNet* getEdge(int index) const { return index >= 0 ? edges[index] : nullptr; }
    HalfEdgeNet* getHalfEdge(int index) const { return index >= 0 ? halfEdges[index] : nullptr; }
    // const std::vector<ConnectorGroup*>& getConnectorGroups() const { return connectorGroups; }
    // BoundNet* getBoundNet() const { return boundNet; }
    int getId() const { return id; }

    // Network operations
    void addVertex(VertexNet* vertex);
    void addEdge(EdgeNet* edge);
    void addHalfEdge(HalfEdgeNet* halfEdge);
    void addFace(FaceNet* face);
    // void addConnectorGroup(ConnectorGroup* group);
    // void setBoundNet(BoundNet* net);

    void removeVertex(VertexNet* vertex);
    void removeEdge(EdgeNet* edge);
    void removeHalfEdge(HalfEdgeNet* halfEdge);
    void removeFace(FaceNet* face);
    void removeSplices();
    // void removeConnectorGroup(ConnectorGroup* group);

    // Conversion methods
    VertexNet* convertVertex(Network* networkB, VertexNet* vertexB);
    EdgeNet* convertEdge(Network* networkB, EdgeNet* edgeB);
    HalfEdgeNet* convertHalfEdge(Network* networkB, HalfEdgeNet* halfEdgeB);
    FaceNet* convertFace(Network* networkB, FaceNet* faceB);
    // ConnectorGroup* convertConnectorGroup(Network* networkB, ConnectorGroup* groupB);

    // Index methods
    int vertexIndex(VertexNet* vertex) const;
    int edgeIndex(EdgeNet* edge) const;
    int halfEdgeIndex(HalfEdgeNet* halfEdge) const;
    int faceIndex(FaceNet* face) const;
    // int connectorGroupIndex(ConnectorGroup* group) const;

    // State queries
    bool isBoundary() const;
    bool isInterior() const;

    // Drawing
    // void draw(View* view, const DrawOptions& options = {});
    // void highlight(View* view, const DrawOptions& options = {});
    void print() const;

    // Copy operations
    // Network* copy() const;

    // Import/Export
    static Network* import(const Json& json, Shape3D* shape = nullptr);
    // Json export() const;

private:
    std::vector<VertexNet*> vertices;
    std::vector<EdgeNet*> edges;
    std::vector<HalfEdgeNet*> halfEdges;
    std::vector<FaceNet*> faces;
    std::vector<ConnectorGroup*> connectorGroups;

    // Boundary vertices and halfEdges.
    std::vector<VertexNet*> bVertices;
    std::vector<HalfEdgeNet*> bHalfEdges;
    int id;

    static int nextId;

    // Helper methods
    /*void drawEmptyNetwork(View* view, const DrawOptions& options) const;
    void drawNetworkElements(View* view, const DrawOptions& options) const;
    static Vec3 combineTangents(const Vec3& u, const Vec3& v);*/
};

} // namespace ms 