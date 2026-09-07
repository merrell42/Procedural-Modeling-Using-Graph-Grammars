#pragma once

#include "../geometry/mesh.h"

class Graph;

// Build a mesh representation of an abstract graph for debugging.
MeshCpp createDebugMesh(const Graph* graph);
