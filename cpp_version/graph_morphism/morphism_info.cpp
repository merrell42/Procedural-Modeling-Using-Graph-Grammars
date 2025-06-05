#include "pch.h"
#include "morphism_info.h"

namespace ms {

NetGraphMapInfo::NetGraphMapInfo(Model* model, Graph* graphB)
    : model(model) , graphB(graphB) {
    verticesB = graphB->getVertices();
    edgesB = graphB->getEdges();
}

NetGraphMapInfo::NetGraphMapInfo(const NetGraphMapInfo& other)
    : model(other.model)
    , graphB(other.graphB)
    , verticesB(other.verticesB)
    , edgesB(other.edgesB) {}

} // namespace ms 