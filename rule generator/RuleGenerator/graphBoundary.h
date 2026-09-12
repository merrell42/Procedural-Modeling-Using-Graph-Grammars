#pragma once

class Graph;

// Walk both outer boundaries. True if the boundaries have the same vertices in the same order.
// This assumes the boundaries are simple cycles, not a more complex graphs.
bool equalBoundaries(Graph* left, Graph* right);

// Set both graphs' boundary vertices to the same outer-walk order so that
// morphism.vertices[i] on one graph is the matching stub on the other.
void alignBoundaries(Graph* left, Graph* right);
