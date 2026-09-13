#pragma once

class Graph;

// True if corresponding bVertices (same index on left and right) have the same
// interior edge type and direction.
bool equalBoundaries(Graph* left, Graph* right);
