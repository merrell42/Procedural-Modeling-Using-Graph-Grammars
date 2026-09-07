#pragma once

class Graph;

// Walk both outer boundaries. True if the boundaries have the same vertices in the same order.
// This assumes the boundaries are simple cycles, not a more complex graphs.
bool equalBoundaries(Graph* left, Graph* right);
