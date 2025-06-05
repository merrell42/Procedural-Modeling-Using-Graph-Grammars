#pragma once
#include <vector>
#include <memory>
#include "morphism_info.h"

namespace ms {

class Graph;
class Edge;

class Morphism {
public:
    Morphism();
    ~Morphism() = default;
    Morphism* copy() const;
    static Morphism* create(const MorphismInfo& info);

    const std::vector<Vertex*>& getVertexBtoA() const { return vertexBtoA; }
    const std::vector<Edge*>& getEdgeBtoA() const { return edgeBtoA; }

    std::vector<Vertex*> vertexBtoA;
    std::vector<Edge*> edgeBtoA;
    std::vector<Face*> faceBtoA;
};

} // namespace ms 