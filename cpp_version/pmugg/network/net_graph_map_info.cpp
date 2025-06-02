#include "pch.h"
#include "net_graph_map_info.h"
// #include "../node/node_stats.h"

namespace ms {

NetGraphMapInfo::NetGraphMapInfo(Model* model, Network* networkB)
    : model(model) , networkB(networkB) {
    verticesB = networkB->getVertices();
    edgesB = networkB->getEdges();
}

NetGraphMapInfo::NetGraphMapInfo(const NetGraphMapInfo& other)
    : model(other.model)
    , networkB(other.networkB)
    , verticesB(other.verticesB)
    , edgesB(other.edgesB) {}

} // namespace ms 