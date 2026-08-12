#include "pch.h"
#include "RuleExporter.h"
#include "isIsomorphic.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/util/util.h"
#include "../../cpp_version/graph_grammar.h"
#include "../../cpp_version/grammar_rules/production_rule.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct PrimitiveGraphs {
	vector<unique_ptr<Graph>> vertexGraphs;
	vector<unique_ptr<Graph>> edgeGraphs;
	// Indexed as 2 * edgeTypeIndex + (spliceOnRight ? 1 : 0). Null if N/A.
	vector<unique_ptr<Graph>> splicedGraphs;
};

struct GlueTrack {
	vector<int> aDest;
	vector<int> bDest;
};

struct FaceBuildInfo {
	FaceType* type = nullptr;
	GraphHalfEdge* hEdges[3] = { nullptr, nullptr, nullptr };
};

VertexType* edgeVertexType() {
	static VertexType* type = new VertexType();
	return type;
}

GraphHalfEdge* firstHalfEdge(GraphVertex* vertex) {
	for (auto* half : vertex->getHalfEdges()) {
		if (half) {
			return half;
		}
	}
	return nullptr;
}

void updateBoundaryVertices(Graph* graph) {
	vector<GraphVertex*> bVertices;
	for (auto* v : graph->getVertices()) {
		if (v && v->getType() == edgeVertexType()) {
			bVertices.push_back(v);
		}
	}
	graph->setBVertices(bVertices);
}

GraphHalfEdge* findBoundaryHalfEdge(const vector<GraphHalfEdge*>& halfEdges) {
	for (auto* half : halfEdges) {
		if (half && !half->getEdge()) {
			return half;
		}
	}
	return nullptr;
}

GraphHalfEdge* findInteriorHalfEdge(const vector<GraphHalfEdge*>& halfEdges) {
	for (auto* half : halfEdges) {
		if (half && half->getEdge()) {
			return half;
		}
	}
	return nullptr;
}

// Like web getConnectors; rotate to a canonical order.
// morphism.halfs stores boundary (edgeless) halfs — what RuleApplier looks up.
void setBoundaryFromWalk(Graph* graph) {
	vector<GraphVertex*> connectors;
	vector<GraphHalfEdge*> boundaryHalfs;
	vector<GraphHalfEdge*> interiorHalfs;
	unordered_set<GraphVertex*> seen;

	for (auto* face : graph->getFaces()) {
		if (!face) {
			continue;
		}
		for (auto* half : face->getOuterHalfEdges()) {
			if (!half || half->getEdge()) {
				continue;
			}
			GraphVertex* vertex = half->getVertex();
			if (!vertex || seen.count(vertex)) {
				continue;
			}
			GraphHalfEdge* interiorHalf = findInteriorHalfEdge(vertex->getHalfEdges());
			GraphHalfEdge* boundaryHalf = findBoundaryHalfEdge(vertex->getHalfEdges());
			if (!interiorHalf || !boundaryHalf) {
				throw runtime_error("setBoundaryFromWalk: boundary vertex missing half-edge");
			}
			seen.insert(vertex);
			connectors.push_back(vertex);
			interiorHalfs.push_back(interiorHalf);
			boundaryHalfs.push_back(boundaryHalf);
		}
	}

	const int n = (int)connectors.size();
	if (n > 1) {
		auto signature = [&](int rot) {
			vector<pair<int, int>> sig;
			sig.reserve(n);
			for (int i = 0; i < n; i++) {
				GraphHalfEdge* half = interiorHalfs[(i + rot) % n];
				EdgeType* eType = half->getEdge()->getType();
				sig.push_back({ eType->getId(), half->getForward() ? 1 : 0 });
			}
			return sig;
		};
		int bestRot = 0;
		auto bestSig = signature(0);
		for (int rot = 1; rot < n; rot++) {
			auto sig = signature(rot);
			if (sig < bestSig) {
				bestSig = std::move(sig);
				bestRot = rot;
			}
		}
		if (bestRot != 0) {
			rotate(connectors.begin(), connectors.begin() + bestRot, connectors.end());
			rotate(interiorHalfs.begin(), interiorHalfs.begin() + bestRot, interiorHalfs.end());
			rotate(boundaryHalfs.begin(), boundaryHalfs.begin() + bestRot, boundaryHalfs.end());
		}
	}

	graph->setBVertices(connectors);
	graph->setBHalfEdges(boundaryHalfs);
}

vector<pair<GraphVertex*, GraphVertex*>> findLoopables(Graph* graph) {
	vector<pair<GraphVertex*, GraphVertex*>> loopables;
	const auto& bVertices = graph->getBVertices();
	unordered_set<GraphVertex*> bVertexSet(bVertices.begin(), bVertices.end());
	for (auto* face : graph->getFaces()) {
		auto outer = face->getOuterHalfEdges();
		for (size_t i = 0; i < outer.size(); i++) {
			auto* v0 = outer[i]->getVertex();
			auto* v1 = outer[(i + 1) % outer.size()]->getVertex();
			if (v0 && v1 && v0 != v1 && bVertexSet.count(v0) && bVertexSet.count(v1)) {
				loopables.push_back({ v0, v1 });
			}
		}
	}
	return loopables;
}

bool halfInGraph(Graph* graph, GraphHalfEdge* half) {
	if (!half) {
		return false;
	}
	const auto& halfEdges = graph->getHalfEdges();
	return find(halfEdges.begin(), halfEdges.end(), half) != halfEdges.end();
}

void removeHalfEdge(GraphHalfEdge* half, Graph* graph) {
	if (!halfInGraph(graph, half)) {
		return;
	}

	auto* face = half->getFace();
	auto* next = half->getNext();
	auto* prev = half->getPrev();

	if (prev && prev != half) {
		if (next && next != half && halfInGraph(graph, next)) {
			prev->connectNext(next);
		} else {
			prev->disconnect();
		}
	}

	graph->removeHalfEdge(half);
	half->disconnectEdge();
	half->disconnect();

	if (face && face->getOuterComponent() == half) {
		if (!next || next == half || !halfInGraph(graph, next)) {
			graph->removeFace(face);
		} else {
			face->replaceHalfEdge(half, next);
		}
	}
}

GraphHalfEdge* glueHalfEdges(GraphHalfEdge* half0, GraphHalfEdge* half1, Graph* graph) {
	if (half0 == half1) {
		graph->removeHalfEdge(half0);
		half0->disconnectEdge();
		auto* face0 = half0->getFace();
		auto* next = half0->getNext();
		auto* prev = half0->getPrev();
		half0->disconnect();
		if (prev) {
			prev->disconnect();
		}
		if (face0 && face0->getOuterComponent() == half0) {
			if (!next || next == half0) {
				graph->removeFace(face0);
			} else {
				face0->replaceHalfEdge(half0, next);
			}
		}
		return nullptr;
	}

	auto* replacement = (new GraphHalfEdge(half0->getForward()))->connectGraph(graph);
	replacement->connectVertex(half0->getVertex(), half0->getVertexIndex());
	replacement->connectEdge(half0->getEdge(), half0->getEdgeIndex());

	auto* next1 = half1->getNext();
	if (next1 && next1 != half1) {
		replacement->connectNext(next1);
	}
	auto* prev0 = half0->getPrev();
	if (prev0 && prev0 != half0) {
		prev0->connectNext(replacement);
	}

	auto* face0 = half0->getFace();
	auto* face1 = half1->getFace();
	GraphFace* face = face0 ? face0 : face1;
	if (face0 && face1 && face0 != face1) {
		face0->mergeInto(face1);
		graph->removeFace(face1);
		face = face0;
	}
	if (face) {
		face->replaceHalfEdge(half0, replacement);
		face->replaceHalfEdge(half1, replacement);
	}

	graph->removeHalfEdge(half0);
	half0->disconnectEdge();
	graph->removeHalfEdge(half1);
	half1->disconnectEdge();

	return replacement;
}

GlueTrack glueVertices(
	GraphVertex* vertexA,
	GraphVertex* vertexB,
	Graph* netA,
	Graph* netB,
	const vector<GraphVertex*>& bVerticesA,
	const vector<GraphVertex*>& bVerticesB,
	bool loopGluing
) {
	auto* halfA = firstHalfEdge(vertexA);
	auto* halfB = firstHalfEdge(vertexB);
	if (!halfA || !halfB) {
		throw runtime_error("glueBVertices: boundary vertex has no half-edge");
	}

	auto* edgeA = halfA->getEdge() ? halfA->getEdge() : halfA->getPrev()->getEdge();
	auto* edgeB = halfB->getEdge() ? halfB->getEdge() : halfB->getPrev()->getEdge();

	auto halfEdgesA = edgeA->getHalfEdges();
	auto halfEdgesB = edgeB->getHalfEdges();
	// Copy before merge: netA->merge clears netB's bVertices vector.
	vector<GraphVertex*> savedBVerticesA(bVerticesA.begin(), bVerticesA.end());
	vector<GraphVertex*> savedBVerticesB(bVerticesB.begin(), bVerticesB.end());

	if (!loopGluing) {
		netA->merge(netB);
	}

	for (auto& halfBs : halfEdgesB) {
		for (auto* halfBSlot : halfBs) {
			if (halfBSlot) {
				halfBSlot->connectEdge(edgeA, halfBSlot->getEdgeIndex());
			}
		}
	}
	netA->removeEdge(edgeB);

	for (size_t i = 0; i < halfEdgesA.size(); i++) {
		bool aOnBoundary = halfEdgesA[i][0] && halfEdgesA[i][0]->getVertex() == vertexA;
		auto* half0 = aOnBoundary ? halfEdgesB[i][0] : halfEdgesA[i][0];
		auto* half1 = aOnBoundary ? halfEdgesA[i][0] : halfEdgesB[i][0];
		if (half0 && half1) {
			GraphHalfEdge* halfToRemove = half0->getNext();
			glueHalfEdges(half0, half1, netA);
			removeHalfEdge(halfToRemove, netA);
		}
	}

	netA->removeVertex(vertexA);
	netA->removeVertex(vertexB);
	updateBoundaryVertices(netA);

	const auto& newBVertices = netA->getBVertices();
	GlueTrack track;
	for (auto* a : savedBVerticesA) {
		track.aDest.push_back(indexOf(newBVertices, a));
	}
	for (auto* b : savedBVerticesB) {
		track.bDest.push_back(indexOf(newBVertices, b));
	}
	return track;
}

pair<unique_ptr<Graph>, GlueTrack> copyAndGlue(
	Graph& graphA,
	int bVertexIndexA,
	Graph& graphB,
	int bVertexIndexB,
	bool loopGluing
) {
	if (loopGluing) {
		auto copyA = unique_ptr<Graph>(graphA.copy());
		const auto& copyBVertices = copyA->getBVertices();
		auto track = glueVertices(
			copyBVertices[bVertexIndexA],
			copyBVertices[bVertexIndexB],
			copyA.get(),
			copyA.get(),
			copyBVertices,
			copyBVertices,
			true
		);
		return { std::move(copyA), track };
	}

	auto copyA = unique_ptr<Graph>(graphA.copy());
	auto copyB = unique_ptr<Graph>(graphB.copy());
	auto track = glueVertices(
		copyA->getBVertices()[bVertexIndexA],
		copyB->getBVertices()[bVertexIndexB],
		copyA.get(),
		copyB.get(),
		copyA->getBVertices(),
		copyB->getBVertices(),
		false
	);
	return { std::move(copyA), track };
}

Graph* createVertexGraph(VertexType* vType) {
	auto* graph = new Graph();
	const auto& halfEdgeTypes = vType->getHalfEdgeTypes();
	unordered_map<int, FaceBuildInfo> faceInfos;
	vector<GraphVertex*> bVertices;
	GraphVertex* center = nullptr;

	const size_t connectionCount = halfEdgeTypes.size();
	vector<vector<int>> connectionFaceIds(connectionCount);
	for (size_t i = 0; i < connectionCount; i++) {
		vector<int> faceIds = { (int)i, (int)((i + 1) % connectionCount) };
		if (!halfEdgeTypes[i].isAtStart) {
			reverse(faceIds.begin(), faceIds.end());
		}
		connectionFaceIds[i] = std::move(faceIds);
	}

	for (size_t connIndex = 0; connIndex < connectionCount; connIndex++) {
		const auto& connection = halfEdgeTypes[connIndex];
		EdgeType* edgeType = connection.edge;
		bool isAtStart = connection.isAtStart;

		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(edgeType);

		auto* bVertex = (new GraphVertex())->connectGraph(graph);
		bVertex->setType(edgeVertexType());
		bVertices.push_back(bVertex);

		if (!center) {
			center = (new GraphVertex())->connectGraph(graph);
			center->setType(vType);
		}

		const auto& faceData = edgeType->getFaceData();
		for (size_t faceIndex = 0; faceIndex < faceData.size(); faceIndex++) {
			const auto& faceDatum = faceData[faceIndex];
			int position = faceDatum.onRight ^ isAtStart;
			bool forward = !faceDatum.onRight;
			auto* half = (new GraphHalfEdge(forward))->connectGraph(graph);
			half->connectEdge(graphEdge, (int)faceIndex);
			int faceId = connectionFaceIds[connIndex][faceIndex];
			auto& faceInfo = faceInfos[faceId];
			if (!faceInfo.type) {
				faceInfo.type = faceDatum.type;
			}
			if (faceInfo.hEdges[position]) {
				throw runtime_error("createVertexGraph: multiple faces with the same ID within a primitive");
			}
			faceInfo.hEdges[position] = half;

			if (position == 0) {
				half->connectVertex(bVertex, -1);
			} else {
				half->connectVertex(center, -1);
				auto* bonusHalf = (new GraphHalfEdge(false))->connectGraph(graph);
				bonusHalf->connectVertex(bVertex, -1);
				faceInfo.hEdges[2] = bonusHalf;
			}
		}
	}

	for (auto& entry : faceInfos) {
		auto& faceInfo = entry.second;
		if (!faceInfo.hEdges[0] || !faceInfo.hEdges[1] || !faceInfo.hEdges[2]) {
			throw runtime_error("createVertexGraph: missing half-edge for face");
		}
		auto* face = (new GraphFace())->connectGraph(graph);
		face->setType(faceInfo.type);
		face->setOuterComponent(faceInfo.hEdges[0]);
		faceInfo.hEdges[0]->connectNext(faceInfo.hEdges[1]);
		faceInfo.hEdges[1]->connectNext(faceInfo.hEdges[2]);
		for (auto* half : faceInfo.hEdges) {
			if (half) {
				half->setFace(face);
			}
		}
	}

	graph->setBVertices(bVertices);
	return graph;
}

Graph* createEdgeGraph(EdgeType* eType) {
	auto* graph = new Graph();

	auto* bVertex0 = (new GraphVertex())->connectGraph(graph);
	bVertex0->setType(edgeVertexType());
	auto* bVertex1 = (new GraphVertex())->connectGraph(graph);
	bVertex1->setType(edgeVertexType());

	auto* graphEdge = (new GraphEdge())->connectGraph(graph);
	graphEdge->setType(eType);

	const auto& faceData = eType->getFaceData();
	for (size_t faceIndex = 0; faceIndex < faceData.size(); faceIndex++) {
		const auto& faceDatum = faceData[faceIndex];
		bool onRight = faceDatum.onRight;
		bool forward = !onRight;

		auto* hStart = (new GraphHalfEdge(forward))->connectGraph(graph);
		auto* hEnd = (new GraphHalfEdge(false))->connectGraph(graph);
		hStart->connectEdge(graphEdge, (int)faceIndex);

		bool start0 = !onRight;
		hStart->connectVertex(start0 ? bVertex0 : bVertex1, -1);
		hEnd->connectVertex(start0 ? bVertex1 : bVertex0, -1);

		auto* face = (new GraphFace())->connectGraph(graph);
		face->setType(faceDatum.type);
		face->setOuterComponent(hStart);
		hStart->connectNext(hEnd);
	}

	vector<GraphVertex*> bVertices = { bVertex0, bVertex1 };
	graph->setBVertices(bVertices);
	return graph;
}

bool isSplitVertexTypeForEdge(VertexType* vType, EdgeType* segmentType) {
	if (!vType || !vType->getSpliced()) {
		return false;
	}
	const auto& hets = vType->getHalfEdgeTypes();
	if (hets.size() != 2) {
		return false;
	}
	if (hets[0].edge != segmentType || hets[1].edge != segmentType) {
		return false;
	}
	return hets[0].isAtStart != hets[1].isAtStart;
}

// Mid-edge vertex created when an edge is split for a splice. Must live in
// primitives->vertexTypes so Graph::exportJson emits kind "v" (not "e").
VertexType* findOrCreateSplitVertexType(Primitives* primitives, EdgeType* segmentType) {
	for (auto* vType : primitives->vertexTypes) {
		if (isSplitVertexTypeForEdge(vType, segmentType)) {
			return vType;
		}
	}

	auto* vertexType = new VertexType();
	// Same half-edge order as Edge::getVertexType (runtime fullSplit).
	vertexType->addHalfEdge(segmentType, true);
	vertexType->addHalfEdge(segmentType, false);
	vertexType->setSpliced(true);
	vertexType->setRuleGeneratorId((int)primitives->vertexTypes.size());
	primitives->vertexTypes.push_back(vertexType);
	return vertexType;
}

EdgeType* findOrCreateSplicedEdgeType(Primitives* primitives, FaceType* faceType) {
	for (auto* eType : primitives->edgeTypes) {
		if (!eType->getSpliced()) {
			continue;
		}
		const auto& faceData = eType->getFaceData();
		bool hasFace = false;
		for (const auto& fd : faceData) {
			if (fd.type == faceType) {
				hasFace = true;
				break;
			}
		}
		if (hasFace) {
			findOrCreateSplitVertexType(primitives, eType);
			return eType;
		}
	}

	// Match createVertexGraph's expected faceData order: onRight=true then onRight=false.
	vector<FaceData> faceData = {
		{ faceType, true },
		{ faceType, false }
	};
	auto* eType = new EdgeType(faceData, Vec3(1, 0, 0), false);
	eType->setSpliced(true);
	eType->setRuleGeneratorId("spliceFace" + to_string((int)primitives->edgeTypes.size()));
	primitives->edgeTypes.push_back(eType);
	findOrCreateSplitVertexType(primitives, eType);
	return eType;
}

// Mid-edge splice site: center is a spliced VertexType for the segment;
// bVertices[0]=start, [1]=end, [2]=splice. Splice spoke uses splicedEdgeType.
Graph* createSplicedVertexGraph(
	Primitives* primitives,
	EdgeType* segmentType,
	bool spliceOnRight,
	bool spliceIsAtStart,
	EdgeType* splicedEdgeType
) {
	auto* graph = new Graph();
	auto* centerType = findOrCreateSplitVertexType(primitives, segmentType);
	auto* center = (new GraphVertex())->connectGraph(graph);
	center->setType(centerType);

	struct Spoke {
		EdgeType* edge = nullptr;
		bool isAtStart = false;
		int bSlot = 0;
	};

	// CCW fan: end, then start; splice on left/right of start→end.
	// !spliceIsAtStart keeps splice.fwd == next.fwd after gluing (see H.json).
	vector<Spoke> ccw;
	const bool spliceSpokeAtStart = !spliceIsAtStart;
	if (!spliceOnRight) {
		ccw = {
			{ segmentType, false, 1 },
			{ segmentType, true, 0 },
			{ splicedEdgeType, spliceSpokeAtStart, 2 }
		};
	} else {
		ccw = {
			{ segmentType, false, 1 },
			{ splicedEdgeType, spliceSpokeAtStart, 2 },
			{ segmentType, true, 0 }
		};
	}

	const size_t connectionCount = ccw.size();
	vector<vector<int>> connectionFaceIds(connectionCount);
	for (size_t i = 0; i < connectionCount; i++) {
		vector<int> faceIds = { (int)i, (int)((i + 1) % connectionCount) };
		if (!ccw[i].isAtStart) {
			reverse(faceIds.begin(), faceIds.end());
		}
		connectionFaceIds[i] = std::move(faceIds);
	}

	vector<GraphVertex*> bVertices(3, nullptr);
	unordered_map<int, FaceBuildInfo> faceInfos;

	for (size_t connIndex = 0; connIndex < connectionCount; connIndex++) {
		const auto& spoke = ccw[connIndex];
		bool isAtStart = spoke.isAtStart;

		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(spoke.edge);

		auto* bVertex = (new GraphVertex())->connectGraph(graph);
		bVertex->setType(edgeVertexType());
		bVertices[spoke.bSlot] = bVertex;

		const auto& faceData = spoke.edge->getFaceData();
		for (size_t faceIndex = 0; faceIndex < faceData.size(); faceIndex++) {
			const auto& faceDatum = faceData[faceIndex];
			int position = faceDatum.onRight ^ isAtStart;
			bool forward = !faceDatum.onRight;
			auto* half = (new GraphHalfEdge(forward))->connectGraph(graph);
			half->connectEdge(graphEdge, (int)faceIndex);
			// Pick angular sector by onRight, not faceData array index — array order varies.
			const int sideIndex = faceDatum.onRight ? 0 : 1;
			int faceId = connectionFaceIds[connIndex][sideIndex];
			auto& faceInfo = faceInfos[faceId];
			if (!faceInfo.type) {
				faceInfo.type = faceDatum.type;
			}
			if (faceInfo.hEdges[position]) {
				throw runtime_error("createSplicedVertexGraph: multiple faces with the same ID within a primitive");
			}
			faceInfo.hEdges[position] = half;

			if (position == 0) {
				half->connectVertex(bVertex, -1);
			} else {
				half->connectVertex(center, -1);
				auto* bonusHalf = (new GraphHalfEdge(false))->connectGraph(graph);
				bonusHalf->connectVertex(bVertex, -1);
				faceInfo.hEdges[2] = bonusHalf;
			}
		}
	}

	for (auto* bVertex : bVertices) {
		if (!bVertex) {
			throw runtime_error("createSplicedVertexGraph: missing boundary vertex");
		}
	}

	for (auto& entry : faceInfos) {
		auto& faceInfo = entry.second;
		if (!faceInfo.hEdges[0] || !faceInfo.hEdges[1] || !faceInfo.hEdges[2]) {
			throw runtime_error("createSplicedVertexGraph: missing half-edge for face");
		}
		auto* face = (new GraphFace())->connectGraph(graph);
		face->setType(faceInfo.type);
		face->setOuterComponent(faceInfo.hEdges[0]);
		faceInfo.hEdges[0]->connectNext(faceInfo.hEdges[1]);
		faceInfo.hEdges[1]->connectNext(faceInfo.hEdges[2]);
		for (auto* half : faceInfo.hEdges) {
			if (half) {
				half->setFace(face);
			}
		}
	}

	graph->setBVertices(bVertices);
	return graph;
}

Graph* getPrimitiveGraph(
	const GraphValues& graphValues,
	size_t index,
	const PrimitiveGraphs& graphs
) {
	if (graphValues.vertexSpliced[index]) {
		const int eTypeIndex = graphValues.vertices[index];
		const int poolIndex = 4 * eTypeIndex
			+ 2 * (graphValues.spliceOnRight[index] ? 1 : 0)
			+ (graphValues.spliceIsAtStart[index] ? 1 : 0);
		if (poolIndex < 0 || poolIndex >= (int)graphs.splicedGraphs.size() ||
			!graphs.splicedGraphs[poolIndex]) {
			throw runtime_error("getPrimitiveGraph: missing spliced primitive");
		}
		return graphs.splicedGraphs[poolIndex].get();
	}
	const bool onBoundary = graphValues.vertexOnBoundary[index];
	const auto& primitives = onBoundary ? graphs.edgeGraphs : graphs.vertexGraphs;
	return primitives[graphValues.vertices[index]].get();
}

PrimitiveGraphs createPrimitiveGraphs(Primitives* primitives, bool buildSpliced) {
	PrimitiveGraphs result;
	result.vertexGraphs.reserve(primitives->vertexTypes.size());
	for (auto* vType : primitives->vertexTypes) {
		result.vertexGraphs.push_back(unique_ptr<Graph>(createVertexGraph(vType)));
	}

	result.edgeGraphs.reserve(primitives->edgeTypes.size());
	for (auto* eType : primitives->edgeTypes) {
		result.edgeGraphs.push_back(unique_ptr<Graph>(createEdgeGraph(eType)));
	}

	if (!buildSpliced) {
		return result;
	}

	const size_t numOriginalEdgeTypes = primitives->edgeTypes.size();
	result.splicedGraphs.resize(numOriginalEdgeTypes * 4);
	for (size_t i = 0; i < numOriginalEdgeTypes; i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		if (eType->getSpliced()) {
			continue;
		}
		for (int side = 0; side < 2; side++) {
			const bool onRight = side == 1;
			FaceType* faceType = nullptr;
			for (const auto& fd : eType->getFaceData()) {
				if (fd.onRight == onRight) {
					faceType = fd.type;
					break;
				}
			}
			if (!faceType) {
				continue;
			}
			EdgeType* splicedEdge = findOrCreateSplicedEdgeType(primitives, faceType);
			for (int atStart = 0; atStart < 2; atStart++) {
				const bool spliceIsAtStart = atStart == 1;
				auto* splicedGraph = createSplicedVertexGraph(
					primitives, eType, onRight, spliceIsAtStart, splicedEdge
				);
				result.splicedGraphs[4 * i + 2 * side + atStart] = unique_ptr<Graph>(splicedGraph);
			}
		}
	}

	// New splice edge types created above need edge-graph prototypes too.
	while (result.edgeGraphs.size() < primitives->edgeTypes.size()) {
		result.edgeGraphs.push_back(
			unique_ptr<Graph>(createEdgeGraph(primitives->edgeTypes[result.edgeGraphs.size()]))
		);
	}
	return result;
}

Graph* releaseInstance(vector<unique_ptr<Graph>>& instances, Graph* graph) {
	for (auto& instance : instances) {
		if (instance.get() == graph) {
			return instance.release();
		}
	}
	throw runtime_error("buildGraphFromValues: result not owned by instances");
}

Graph* buildGraphFromValues(
	const GraphValues& graphValues,
	const PrimitiveGraphs& graphs
) {
	if (graphValues.edges.empty() && graphValues.vertices.size() == 0) {
		return new Graph();
	}

	vector<unique_ptr<Graph>> instances;
	instances.reserve(graphValues.vertices.size());
	for (size_t i = 0; i < graphValues.vertices.size(); i++) {
		Graph* prototype = getPrimitiveGraph(graphValues, i, graphs);
		instances.push_back(unique_ptr<Graph>(prototype->copy()));
	}

	unordered_map<string, string> matchToNetwork;
	unordered_map<string, string> networkToMatch;
	unordered_map<int, Graph*> networkMap;

	for (size_t i = 0; i < instances.size(); i++) {
		auto* graph = instances[i].get();
		const auto& bVertices = graph->getBVertices();
		for (size_t j = 0; j < bVertices.size(); j++) {
			string matchKey = to_string(i) + "," + to_string(j);
			string netKey = to_string(graph->getId()) + "," + to_string(j);
			matchToNetwork[matchKey] = netKey;
			networkToMatch[netKey] = matchKey;
		}
		networkMap[graph->getId()] = graph;
	}

	vector<array<int, 4>> edgeQueue = graphValues.edges;
	Graph* finalResult = nullptr;

	if (edgeQueue.empty()) {
		Graph* result = instances[0].release();
		setBoundaryFromWalk(result);
		return result;
	}

	while (!edgeQueue.empty()) {
		vector<array<int, 4>> nextQueue;
		for (const auto& edge : edgeQueue) {
			int vertexA = edge[0];
			int bVertexIndexA = edge[1];
			int vertexB = edge[2];
			int bVertexIndexB = edge[3];

			string keyA = to_string(vertexA) + "," + to_string(bVertexIndexA);
			string keyB = to_string(vertexB) + "," + to_string(bVertexIndexB);
			auto netKeyA = matchToNetwork[keyA];
			auto netKeyB = matchToNetwork[keyB];
			auto commaA = netKeyA.find(',');
			auto commaB = netKeyB.find(',');
			int netA = stoi(netKeyA.substr(0, commaA));
			int nBVertA = stoi(netKeyA.substr(commaA + 1));
			int netB = stoi(netKeyB.substr(0, commaB));
			int nBVertB = stoi(netKeyB.substr(commaB + 1));

			auto* graphA = networkMap[netA];
			auto* graphB = networkMap[netB];

			if (netA == netB) {
				auto loopables = findLoopables(graphA);
				bool canLoop = false;
				auto* bVertexA = graphA->getBVertices()[nBVertA];
				auto* bVertexB = graphA->getBVertices()[nBVertB];
				for (const auto& loopable : loopables) {
					if ((loopable.first == bVertexA && loopable.second == bVertexB) ||
						(loopable.second == bVertexA && loopable.first == bVertexB)) {
						canLoop = true;
						break;
					}
				}
				if (!canLoop) {
					nextQueue.push_back(edge);
					continue;
				}
			}

			auto outcome = copyAndGlue(*graphA, nBVertA, *graphB, nBVertB, netA == netB);
			auto& track = outcome.second;
			Graph* merged = outcome.first.get();
			int mergedId = merged->getId();

			for (size_t index = 0; index < track.aDest.size(); index++) {
				if (track.aDest[index] >= 0) {
					string oldMatchKey = networkToMatch[to_string(netA) + "," + to_string(index)];
					string newNetKey = to_string(mergedId) + "," + to_string(track.aDest[index]);
					matchToNetwork[oldMatchKey] = newNetKey;
					networkToMatch[newNetKey] = oldMatchKey;
				}
			}
			if (netA != netB) {
				for (size_t index = 0; index < track.bDest.size(); index++) {
					if (track.bDest[index] >= 0) {
						string oldMatchKey = networkToMatch[to_string(netB) + "," + to_string(index)];
						string newNetKey = to_string(mergedId) + "," + to_string(track.bDest[index]);
						matchToNetwork[oldMatchKey] = newNetKey;
						networkToMatch[newNetKey] = oldMatchKey;
					}
				}
				networkMap.erase(netB);
			}

			finalResult = merged;
			networkMap[mergedId] = finalResult;
			instances.push_back(std::move(outcome.first));
		}

		if (edgeQueue.size() == nextQueue.size()) {
			throw runtime_error("buildGraphFromValues: cannot glue graph");
		}
		edgeQueue = std::move(nextQueue);
	}

	if (!finalResult) {
		throw runtime_error("buildGraphFromValues: no result");
	}
	setBoundaryFromWalk(finalResult);
	return releaseInstance(instances, finalResult);
}

bool isDuplicateGraph(
	Graph* graph,
	const vector<int>& vertexTypeIds,
	const vector<unique_ptr<Graph>>& existingGraphs,
	const vector<vector<int>>& existingVertexTypeIds
) {
	for (size_t j = 0; j < existingGraphs.size(); j++) {
		if (isIsomorphic(graph, vertexTypeIds, existingGraphs[j].get(), existingVertexTypeIds[j])) {
			return true;
		}
	}
	return false;
}

bool isLeafDigonFace(GraphFace* face) {
	auto halves = face->getOuterHalfEdges();
	if (halves.size() != 2 || !halves[0] || !halves[1]) {
		return false;
	}
	GraphEdge* edge = halves[0]->getEdge();
	if (!edge || halves[1]->getEdge() != edge) {
		return false;
	}
	return halves[0]->getNext() == halves[1] && halves[1]->getNext() == halves[0];
}

bool loopsAreValid(Graph* graph) {
	bool hasOuterLoop = false;
	for (int i = 0; i < graph->getFaces().size(); i++) {
		auto* face = graph->getFaces()[i];
		if (face->isLoopy()) {
			// Leaf edges are a digon: both FaceData sides share one face.
			// Shipped Unity grammars (e.g. 2D Branches/line.json) use this.
			if (isLeafDigonFace(face)) {
				continue;
			}
			int turns = face->computeTurns();
			// An outer loop has 1 turn, an inner loop has -1 turn.
			bool isOuterLoop = (turns == 1);
			bool isInnerLoop = (turns == -1);
			if (!isOuterLoop && !isInnerLoop) {
				// Any other number of turns is invalid.
				return false;
			}
			// There can be many inner loops, but only one outer loop.
			if (isOuterLoop) {
				if (hasOuterLoop) {
					return false;
				}
				hasOuterLoop = true;
			}
		}
	}
	return true;
}

void exportRule(
	GraphGrammar* grammar,
	Graph* leftGraph,
	Graph* rightGraph
) {
	try {
		const int numLeftVertices = (int)leftGraph->getVertices().size();
		const int numLeftEdges = (int)leftGraph->getEdges().size();
		const int numRightVertices = (int)rightGraph->getVertices().size();
		const int numRightEdges = (int)rightGraph->getEdges().size();
		const bool leftEmpty = numLeftVertices == 0 && numLeftEdges == 0;
		const bool rightEmpty = numRightVertices == 0 && numRightEdges == 0;
        // If a graph is empty, it should go first.
		vector<Graph*> graphs = rightEmpty
			? vector<Graph*>{ rightGraph, leftGraph }
			: vector<Graph*>{ leftGraph, rightGraph };
		// End graphs are start graphs with splices removed (ProductionRule ctor).
		ProductionRule* rule = new ProductionRule(graphs);
		if (leftEmpty || rightEmpty) {
			grammar->addStarterRule(rule);
		} else {
			grammar->addRule(rule);
		}

		cout << "    exported "
			<< (leftEmpty || rightEmpty ? "starter" : "normal")
			<< " rule: left graph ("
			<< numLeftVertices << " vertices, "
			<< numLeftEdges << " edges), right graph ("
			<< numRightVertices << " vertices, "
			<< numRightEdges << " edges)\n";
	} catch (const exception& e) {
		cerr << "    export failed: " << e.what() << "\n";
	}
}

void RuleExporter::exportGroups(
	GraphGrammar& grammar,
	const vector<GraphGroup>& groups,
	const vector<TemplateMatcher>& matchers,
	Primitives* primitives
) {
	bool needSpliced = false;
	for (const auto& matcher : matchers) {
		for (const auto& vertex : matcher.templateGraph.vertices) {
			if (vertex.spliced) {
				needSpliced = true;
				break;
			}
		}
		if (needSpliced) {
			break;
		}
	}

	auto primitiveGraphs = createPrimitiveGraphs(primitives, needSpliced);
	for (const auto& group : groups) {
		const int numGraphs = (int)group.graphIndices.size();
		vector<vector<unique_ptr<Graph>>> graphs(numGraphs);
		vector<vector<vector<int>>> vertexTypeIds(numGraphs);

		for (int i = 0; i < numGraphs; i++) {
			for (int index : group.graphIndices[i]) {
				try {
					auto graphValues = matchers[i].getGraphValues(index);
					auto graphA = unique_ptr<Graph>(buildGraphFromValues(graphValues, primitiveGraphs));
					auto vertexTypeIdsA = getVertexTypeIds(graphA.get());
					if (loopsAreValid(graphA.get()) &&
						!isDuplicateGraph(graphA.get(), vertexTypeIdsA, graphs[i], vertexTypeIds[i])) {
						graphs[i].push_back(std::move(graphA));
						vertexTypeIds[i].push_back(std::move(vertexTypeIdsA));
					}
				} catch (const exception& e) {
					cerr << "    buildGraphFromValues failed (graph " << i
						 << " match " << index << "): " << e.what() << "\n";
				}
			}
		}

		// Assumes there are only two graphs in the template set.
		for (const auto& left : graphs[0]) {
			for (const auto& right : graphs[1]) {
				try {
					exportRule(&grammar, left->copy(), right->copy());
				} catch (const exception& e) {
					cerr << "    exportRule copy failed: " << e.what() << "\n";
				}
			}
		}
	}
}
