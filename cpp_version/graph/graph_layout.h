#pragma once

#include "../geometry/mesh.h"

class Graph;

// Lay out an abstract graph and export it as mesh geometry scaled to a unit cube.
MeshCpp exportGraphMesh(const Graph* graph);
