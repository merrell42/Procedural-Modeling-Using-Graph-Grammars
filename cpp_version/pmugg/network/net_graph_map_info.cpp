#include "net_graph_map_info.h"
// #include "../node/node_stats.h"

namespace ms {

NetGraphMapInfo::NetGraphMapInfo(Model* model, Network* networkB)
    : model(model)
    , networkB(networkB)
{
    // Initialize vectors with vertices and edges from netB's interior
    verticesB = networkB->getVertices();
    edgesB = networkB->getEdges();
}

// Copy constructor
NetGraphMapInfo::NetGraphMapInfo(const NetGraphMapInfo& other)
    : model(other.model)
    , networkB(other.networkB)
    , verticesB(other.verticesB)
    , edgesB(other.edgesB)
{
    // Copy other members as needed
    /*
    indexA = other.indexA;
    indexB = other.indexB;
    getVertexSignature = other.getVertexSignature;
    getEdgeSignature = other.getEdgeSignature;
    numEdgesA = other.numEdgesA;
    numEdgesB = other.numEdgesB;
    */
}

//std::string NetGraphMapInfo::vSignA(int index) const {
//    return networkB->getConnectors()[index]->signature();
//}
//
//std::string NetGraphMapInfo::vSignB(int index) const {
//    return networkB->getConnectors()[index]->signature();
//}

//std::string NetGraphMapInfo::eSignA(int index) const {
//    // Implement when needed
//    // return getEdgeSignature(graphA->getEdges()[index]);
//    return "";
//}
//
//std::string NetGraphMapInfo::eSignB(int index) const {
//    // Implement when needed
//    // return getEdgeSignature(graphB->getEdges()[index]);
//    return "";
//}

} // namespace ms 