#pragma once
#include <vector>
#include <string>
#include "../graph/graph.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph_edge.h"
#include "../graph_drawing/model.h"

namespace ms {

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
    std::vector<GraphVertex*> verticesB;
    std::vector<GraphEdge*> edgesB;
};

} // namespace ms 