#pragma once
#include <vector>
#include <string>
#include "network.h"
#include "../graph_drawing/model.h"
#include "vertex_net.h"
#include "edge_net.h"

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