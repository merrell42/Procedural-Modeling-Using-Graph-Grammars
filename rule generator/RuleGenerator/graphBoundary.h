#pragma once

#include <string>

class Graph;

// GlueTrack already chose the remaining stubs. These put them in face-cycle
// order so morphism.vertices[i] is the same place on both graphs.

void alignBoundaries(Graph* left, Graph* right);

// Rotation-invariant key of that cycle (edge type + direction at each stub).
std::string boundaryTypeKey(Graph* graph);
