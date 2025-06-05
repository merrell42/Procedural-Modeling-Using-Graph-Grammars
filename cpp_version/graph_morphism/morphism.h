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

    const vector<Vertex*>& getVertexBtoA() const { return vertexBtoA; }
    const vector<Edge*>& getEdgeBtoA() const { return edgeBtoA; }

    vector<Vertex*> vertexBtoA;
    vector<Edge*> edgeBtoA;
    vector<Face*> faceBtoA;
};

} // namespace ms 