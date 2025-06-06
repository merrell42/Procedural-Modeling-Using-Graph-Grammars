#pragma once
#include <vector>
#include <string>
#include "../graph/graph.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph_edge.h"
#include "../graph_drawing/model.h"

class Graph;
class NodeStats;
class Vertex;
class Edge;

class MorphismInfo {
public:
    MorphismInfo(Model* model, Graph* graphB);
    MorphismInfo(const MorphismInfo& other);

    Model* model;
    Graph* graphB;  
    vector<GraphVertex*> verticesB;
    vector<GraphEdge*> edgesB;
};

