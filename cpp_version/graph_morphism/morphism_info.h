#pragma once
#include <vector>
#include <string>
#include "../graph/graph.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph_edge.h"
#include "../graph_drawing/model.h"

namespace ms {

class Network;
class NodeStats;
class Vertex;
class Edge;

class NetGraphMapInfo {
public:
    NetGraphMapInfo(Model* model, Network* networkB);
    NetGraphMapInfo(const NetGraphMapInfo& other);

    Model* model;
    Network* networkB;  
    std::vector<VertexNet*> verticesB;
    std::vector<EdgeNet*> edgesB;
};

} // namespace ms 