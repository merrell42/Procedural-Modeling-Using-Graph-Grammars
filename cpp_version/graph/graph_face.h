#pragma once
#include <vector>
#include "../third_party/json.h"
#include "../primitives/face_type.h"

using Json = nlohmann::json;

namespace ms {

class GraphHalfEdge;
class Graph;
class FaceType;

class GraphFace {
public:
    GraphFace();
    void import(const Json& json);

    GraphHalfEdge* getOuterComponent() const;
    const std::vector<GraphHalfEdge*>& getInnerComponents() const;
    Graph* getGraph() const;
    FaceType* getType() const { return type; }
    
    GraphFace* connectNet(Graph* graph);
    void connectOuter(const std::vector<GraphHalfEdge*>& halfEdges);
    void makeInner(GraphHalfEdge* halfEdge);
    void copyConnection(const GraphFace* copy);
    void setType(FaceType* type_) { type = type_;}
    
    static std::vector<GraphHalfEdge*> getConnectedHalfEdges(GraphHalfEdge* start);
    std::vector<GraphHalfEdge*> getOuterHalfEdges() const;
    std::vector<GraphHalfEdge*> getInnerHalfEdges() const;
    std::vector<GraphHalfEdge*> getHalfEdges() const;
    void replaceHalfEdge(GraphHalfEdge* a, GraphHalfEdge* b, bool force);
    bool isLoopy() const;
    bool inGraph() const;

private:
    GraphHalfEdge* outerComponent;
    std::vector<GraphHalfEdge*> innerComponents;
    FaceType* type;
    Graph* graph;
    int id;
};

} // namespace ms 