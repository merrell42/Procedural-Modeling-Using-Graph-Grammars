#include "pch.h"
#include "CreateGroundRule.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_face.h"
#include "../../cpp_version/grammar_rules/production_rule.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"
#include "../../cpp_version/util/util.h"

#include <array>
#include <iostream>
#include <vector>

using namespace std;

namespace {

// If the vertex is the corner of a ground face, return the face.
FaceType* groundFaceOf(VertexType* vertexType) {
	const auto& halfEdges = vertexType->getHalfEdgeTypes();
	if (halfEdges.size() != 2) {
		return nullptr;
	}
	auto edge0 = halfEdges[0].edge;
	auto edge1 = halfEdges[1].edge;
	if (!edge0 || edge0->getFaceData().size() != 1 ||
		!edge1 || edge1->getFaceData().size() != 1) {
		return nullptr;
	}
	FaceType* face0 = edge0->getFaceData()[0].type;
	FaceType* face1 = edge1->getFaceData()[0].type;
	if (face0 != face1) {
		return nullptr;
	}
	return face0;
}

// Put the four ground corners in cycle order. Start at one corner, follow
// the shared edge to the next corner, then that corner's other half-edge,
// until we return to the start.
bool orderCorners(
	const vector<VertexType*>& corners,
	array<VertexType*, 4>& outCorners,
	array<EdgeType*, 4>& outEdges
) {
	if (corners.size() != 4) {
		return false;
	}

	VertexType* current = corners[0];
	EdgeType* edge = current->getHalfEdgeTypes()[0].edge;
	for (int i = 0; i < 4; i++) {
		outCorners[i] = current;
		outEdges[i] = edge;
		VertexType* next = nullptr;
		EdgeType* nextEdge = nullptr;
		for (VertexType* corner : corners) {
			if (corner == current) {
				continue;
			}
			const auto& halfEdges = corner->getHalfEdgeTypes();
			if (halfEdges[0].edge == edge) {
				next = corner;
				nextEdge = halfEdges[1].edge;
			} else if (halfEdges[1].edge == edge) {
				next = corner;
				nextEdge = halfEdges[0].edge;
			}
		}
		if (!next) {
			return false;
		}
		current = next;
		edge = nextEdge;
	}
	return current == corners[0];
}

Graph* buildRectangleGraph(
	const array<VertexType*, 4>& corners,
	const array<EdgeType*, 4>& edges,
	FaceType* face
) {
	Graph* graph = new Graph();
	array<GraphHalfEdge*, 4> halfEdges;
	for (int i = 0; i < 4; i++) {
		auto* vertex = (new GraphVertex())->connectGraph(graph);
		vertex->setType(corners[i]);
		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(edges[i]);
		halfEdges[i] = (new GraphHalfEdge(false))->connectGraph(graph);
		halfEdges[i]->connectEdge(graphEdge, 0);
		halfEdges[i]->connectVertex(vertex, 0);
	}

	auto* graphFace = (new GraphFace())->connectGraph(graph);
	graphFace->setType(face);
	graphFace->setOuterComponent(halfEdges[0]);
	for (int i = 0; i < 4; i++) {
		halfEdges[i]->setFace(graphFace);
		halfEdges[i]->connectNext(halfEdges[(i + 1) % 4]);
	}
	return graph;
}

}

ProductionRule* createGroundRule(Primitives* primitives) {
	vector<VertexType*> corners;
	FaceType* groundFace = nullptr;
	for (VertexType* vertexType : primitives->vertexTypes) {
		FaceType* vertexFace = groundFaceOf(vertexType);
		if (vertexFace) {
			if (!groundFace) {
				groundFace = vertexFace;
			}
			if (vertexFace == groundFace) {
				corners.push_back(vertexType);
			}
		}
	}

	array<VertexType*, 4> orderedCorners;
	array<EdgeType*, 4> orderedEdges;
	if (!orderCorners(corners, orderedCorners, orderedEdges)) {
		cout << "  ground rule: skipped\n";
		return nullptr;
	}

	Graph* emptyGraph = new Graph();
	Graph* rectangleGraph = buildRectangleGraph(orderedCorners, orderedEdges, groundFace);

	cout << "  ground rule: created rectangle with vertex types [";
	for (int i = 0; i < 4; i++) {
		cout << indexOf(primitives->vertexTypes, orderedCorners[i]) << (i < 3 ? ", " : "");
	}
	cout << "]\n";
	return new ProductionRule({ emptyGraph, rectangleGraph });
}
