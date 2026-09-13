#pragma once

class Graph;

// True if corresponding bVertices (same index on left and right) have the same
// interior edge type and direction.
bool equalBoundaries(Graph* left, Graph* right);

// Put both graphs' bVertices in outer-walk order, rotated so slot i matches.
void alignBoundaries(Graph* left, Graph* right);

// Reorder source bVertices to follow the boundaryKey sequence on pattern.
void matchBoundaryOrder(Graph* source, Graph* pattern);

// Set source bVertices by finding each pattern boundary key on some source vertex.
// Returns false if any pattern slot has no unused matching vertex.
bool assignBoundaryVerticesFromPattern(Graph* source, Graph* pattern);

// Like assignBoundaryVerticesFromPattern, but only uses vertices that still exist
// after removeSplices (interior splice vertices are excluded).
bool assignBoundaryVerticesFromSurvivors(Graph* source, Graph* pattern);
