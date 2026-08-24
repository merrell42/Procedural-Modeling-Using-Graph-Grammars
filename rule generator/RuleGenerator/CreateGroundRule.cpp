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
#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

namespace {

constexpr double kParallelDot = 0.99;
constexpr double kPerpendicularDot = 0.01;

bool areParallel(const Vec3& a, const Vec3& b) {
	return abs(a.dot(b)) >= kParallelDot;
}

bool arePerpendicular(const Vec3& a, const Vec3& b) {
	return abs(a.dot(b)) <= kPerpendicularDot;
}

bool hasOneFace(const HalfEdgeType& halfEdge) {
	return halfEdge.edge && halfEdge.edge->getFaceData().size() == 1;
}

// A ground corner: two half-edges, one shared face, a right angle.
FaceType* groundFaceOf(VertexType* vertexType) {
	const auto& halfEdges = vertexType->getHalfEdgeTypes();
	if (halfEdges.size() != 2) {
		return nullptr;
	}
	if (!hasOneFace(halfEdges[0]) || !hasOneFace(halfEdges[1])) {
		return nullptr;
	}
	FaceType* face0 = halfEdges[0].edge->getFaceData()[0].type;
	FaceType* face1 = halfEdges[1].edge->getFaceData()[0].type;
	if (!face0 || face0 != face1) {
		return nullptr;
	}
	if (!arePerpendicular(halfEdges[0].edge->getDir(), halfEdges[1].edge->getDir())) {
		return nullptr;
	}
	return face0;
}

const HalfEdgeType* otherHalfEdge(VertexType* vertexType, EdgeType* used) {
	for (const auto& halfEdge : vertexType->getHalfEdgeTypes()) {
		if (halfEdge.edge != used) {
			return &halfEdge;
		}
	}
	return nullptr;
}

VertexType* oppositeCorner(
	const vector<VertexType*>& corners,
	VertexType* from,
	const HalfEdgeType& outgoing
) {
	for (VertexType* corner : corners) {
		if (corner == from) {
			continue;
		}
		for (const auto& halfEdge : corner->getHalfEdgeTypes()) {
			if (halfEdge.edge == outgoing.edge && halfEdge.isAtStart != outgoing.isAtStart) {
				return corner;
			}
		}
	}
	return nullptr;
}

bool isRectangle(const array<EdgeType*, 4>& edges) {
	return areParallel(edges[0]->getDir(), edges[2]->getDir()) &&
		areParallel(edges[1]->getDir(), edges[3]->getDir()) &&
		arePerpendicular(edges[0]->getDir(), edges[1]->getDir());
}

bool uniqueCorners(const array<VertexType*, 4>& corners) {
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			if (corners[i] == corners[j]) {
				return false;
			}
		}
	}
	return true;
}

// Follow half-edges from a start corner around the four corners of a rectangle.
bool walkRectangle(
	const vector<VertexType*>& corners,
	array<VertexType*, 4>& outCorners,
	array<EdgeType*, 4>& outEdges
) {
	if (corners.size() < 4) {
		return false;
	}

	for (VertexType* start : corners) {
		const auto& startHalfEdges = start->getHalfEdgeTypes();
		for (int choice = 0; choice < 2; choice++) {
			VertexType* current = start;
			const HalfEdgeType* outgoing = &startHalfEdges[choice];
			array<VertexType*, 4> walkedCorners;
			array<EdgeType*, 4> walkedEdges;
			bool ok = true;
			for (int i = 0; i < 4; i++) {
				walkedCorners[i] = current;
				walkedEdges[i] = outgoing->edge;
				VertexType* next = oppositeCorner(corners, current, *outgoing);
				if (!next) {
					ok = false;
					break;
				}
				outgoing = otherHalfEdge(next, outgoing->edge);
				if (!outgoing) {
					ok = false;
					break;
				}
				current = next;
			}
			if (ok && current == start && uniqueCorners(walkedCorners) && isRectangle(walkedEdges)) {
				outCorners = walkedCorners;
				outEdges = walkedEdges;
				return true;
			}
		}
	}
	return false;
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
	FaceType* face = nullptr;
	for (VertexType* vertexType : primitives->vertexTypes) {
		FaceType* vertexFace = groundFaceOf(vertexType);
		if (!vertexFace) {
			continue;
		}
		if (!face) {
			face = vertexFace;
		} else if (vertexFace != face) {
			continue;
		}
		corners.push_back(vertexType);
	}

	array<VertexType*, 4> orderedCorners;
	array<EdgeType*, 4> orderedEdges;
	if (!walkRectangle(corners, orderedCorners, orderedEdges)) {
		cout << "  ground rule: skipped\n";
		return nullptr;
	}

	Graph* emptyGraph = new Graph();
	Graph* rectangleGraph = buildRectangleGraph(orderedCorners, orderedEdges, face);

	cout << "  ground rule: created rectangle with vertex types [";
	for (int i = 0; i < 4; i++) {
		if (i > 0) {
			cout << ", ";
		}
		cout << indexOf(primitives->vertexTypes, orderedCorners[i]);
	}
	cout << "]\n";
	return new ProductionRule({ emptyGraph, rectangleGraph });
}
