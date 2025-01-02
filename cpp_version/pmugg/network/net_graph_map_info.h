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
    // Constructor
    NetGraphMapInfo(Model* model, Network* networkB);
    
    // Copy constructor
    NetGraphMapInfo(const NetGraphMapInfo& other);
    
    // Member functions
    //std::string vSignA(int index) const;
    //std::string vSignB(int index) const;
    //std::string eSignA(int index) const;
    //std::string eSignB(int index) const;
    
    // Data members
    Model* model;
    Network* networkB;  
    std::vector<VertexNet*> verticesB;
    std::vector<EdgeNet*> edgesB;
    
    // Commented out members for future implementation
    /*
    int indexA;
    int indexB;
    std::function<std::string(Vertex*)> getVertexSignature;
    std::function<std::string(Edge*)> getEdgeSignature;
    size_t numEdgesA;
    size_t numEdgesB;
    */
};

} // namespace ms 