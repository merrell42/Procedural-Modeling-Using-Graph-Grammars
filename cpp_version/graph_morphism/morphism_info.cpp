#include "pch.h"
#include "morphism_info.h"



MorphismInfo::MorphismInfo(Model* model, Graph* graphB)
    : model(model) , graphB(graphB) {
    verticesB = graphB->getVertices();
    edgesB = graphB->getEdges();
}

MorphismInfo::MorphismInfo(const MorphismInfo& other)
    : model(other.model)
    , graphB(other.graphB)
    , verticesB(other.verticesB)
    , edgesB(other.edgesB) {}

