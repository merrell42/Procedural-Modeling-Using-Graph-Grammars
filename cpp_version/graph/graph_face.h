#pragma once
#include <vector>
#include <iosfwd>
#include "../third_party/json.h"
#include "../primitives/face_type.h"

using Json = nlohmann::json;

class GraphHalfEdge;
class Graph;
class FaceType;

class GraphFace {
public:
    GraphFace();
    void import(const Json& json);
    void binaryDeserialize(std::istream& in);
    Json exportJson(const vector<GraphHalfEdge*>& graphHalfEdges) const;

    GraphHalfEdge* getOuterComponent() const;
    Graph* getGraph() const;
    FaceType* getType() const { return type; }
    
    GraphFace* connectGraph(Graph* graph);
    void setType(FaceType* newType);
    void setOuterComponent(GraphHalfEdge* half);
    
    static vector<GraphHalfEdge*> getConnectedHalfEdges(const GraphHalfEdge* start);
    vector<GraphHalfEdge*> getOuterHalfEdges() const;
    void replaceHalfEdge(GraphHalfEdge* a, GraphHalfEdge* b);
    void mergeInto(GraphFace* other);
    bool isLoopy() const;

private:
    GraphHalfEdge* outerComponent;
    // Inner components are not currently used, but they may be needed for holes in the future.
    // vector<GraphHalfEdge*> innerComponents;
    FaceType* type;
    Graph* graph;
    int id;
};

