#pragma once
#include <vector>
#include <memory>
#include "morphism_info.h"

class Graph;
class Edge;

class Morphism {
public:
    Morphism();
    ~Morphism() = default;
    Morphism* copy() const;
    static Morphism* create(const MorphismInfo& info);

    const vector<Vertex*>& getVertexBtoA() const;
    const vector<Edge*>& getEdgeBtoA() const;

    vector<Vertex*> vertexBtoA;
    vector<Edge*> edgeBtoA;
    vector<Face*> faceBtoA;
};

