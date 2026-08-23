#pragma once

#include <vector>

class VertexType;

// Reorder each vertex type's halfEdgeTypes so they are ordered correctly.
// This is necessary for matching to the template graphs, but not necessary
// when generating shapes with the grammar.
void fixHalfEdgeOrder(const std::vector<VertexType*>& vertexTypes);
