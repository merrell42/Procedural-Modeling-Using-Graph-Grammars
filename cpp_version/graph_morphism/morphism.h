#pragma once
#include <vector>
#include <memory>
#include "../memory_counter.h"
#include "morphism_info.h"

class Graph;
class Edge;

// A graph morphism is a mapping from a graph to a graph drawing.
class Morphism {
public:
    Morphism();
    ~Morphism();
    static Morphism* create(const MorphismInfo& info);

    const vector<Vertex*>& getVertexBtoA() const;
    const vector<Edge*>& getEdgeBtoA() const;

    vector<Vertex*> vertexBtoA;
    vector<Edge*> edgeBtoA;
    vector<Face*> faceBtoA;
};

