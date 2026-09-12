#include "pch.h"
#include "RuleExporter.h"
#include "isIsomorphic.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_face.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/util/util.h"
#include "../../cpp_version/graph_grammar.h"
#include "../../cpp_version/grammar_rules/production_rule.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct GlueTrack {
	vector<int> aDest;
	vector<int> bDest;
};

struct HalfEdgeFaceSlot {
	double angle = 0.0;
	bool intoVertex = false;
	GraphHalfEdge* halfEdge = nullptr;
};

enum PartnerDirection {
	NEXT,
	PREV
};

// Sort half-edge face slots by angle.
static bool compareHalfEdgeFaceSlots(const HalfEdgeFaceSlot& a, const HalfEdgeFaceSlot& b) {
	if (a.angle != b.angle) {
		return a.angle < b.angle;
	}
	if (a.intoVertex != b.intoVertex) {
		return a.intoVertex < b.intoVertex;
	}
	return false;
}

// We cannot decide the order of two half-edges based on angle if they have the same angle.
// Instead, we look at the previous slot. intoVertex should alternate between true and false.
// Choose the order that accomplishes this.
static void orderSameAnglePairs(vector<HalfEdgeFaceSlot>& slots) {
	const size_t n = slots.size();
	if (n < 2) {
		return;
	}
	for (size_t i = 0; i + 1 < n; ) {
		if (slots[i].angle == slots[i + 1].angle) {
			const size_t prev = (i + n - 1) % n;
			const bool prevIntoVertex = slots[prev].intoVertex;
			// If the current slot matches the previous slot, swap the current and next slots.
			if (slots[i].intoVertex == prevIntoVertex) {
				swap(slots[i], slots[i + 1]);
			}
			i += 2;
		} else {
			i++;
		}
	}
}

// Find the first pair of slots with the same angle, then pick the direction
// that does not make them partners.
static PartnerDirection partnerDirection(const vector<HalfEdgeFaceSlot>& halfEdgeSlots) {
	const size_t n = halfEdgeSlots.size();
	if (n < 2) {
		return NEXT;
	}
	for (size_t i = 0; i < n; i++) {
		const size_t nextIndex = (i + 1) % n;
		if (halfEdgeSlots[i].angle == halfEdgeSlots[nextIndex].angle) {
			// We are trying to find the partner direction for inward half-edges.
			// If the next slot has the same angle, we go in the opposite direction PREV.
			return halfEdgeSlots[i].intoVertex ? PREV : NEXT;
		}
	}
	return NEXT;
}

// Find the outward facing partner half-edge for a slot facing into the vertex.
// Slots are sorted by increasing face angle (CCW on the face).
static GraphHalfEdge* findOutwardPartner(
	const vector<HalfEdgeFaceSlot>& halfEdgeSlots,
	size_t inwardIndex,
	PartnerDirection direction
) {
	const size_t n = halfEdgeSlots.size();
	const size_t partnerIndex = direction == PREV
		? (inwardIndex + n - 1) % n
		: (inwardIndex + 1) % n;
	const auto& partnerSlot = halfEdgeSlots[partnerIndex];
	if (!partnerSlot.intoVertex) {
		return partnerSlot.halfEdge;
	}
	throw runtime_error("createVertexGraph: no outOfVertex partner for intoVertex slot");
}

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

void updateBoundaryHalfEdges(Graph* graph) {
	vector<GraphHalfEdge*> bHalfEdges;
	for (auto* bVertex : graph->getBVertices()) {
		auto* bHalf = findBoundaryHalfEdge(bVertex->getHalfEdges());
		if (!bHalf) {
			throw runtime_error("updateBoundaryHalfEdges: boundary vertex has no half-edge on the boundary");
		}
		bHalfEdges.push_back(bHalf);
	}
	graph->setBHalfEdges(bHalfEdges);
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

bool parseIndexPair(const string& key, int& a, int& b) {
	auto comma = key.find(',');
	if (comma == string::npos) {
		return false;
	}
	a = stoi(key.substr(0, comma));
	b = stoi(key.substr(comma + 1));
	return true;
}

GraphVertex* liveStub(
	const unordered_map<string, string>& matchToGraph,
	Graph* result,
	int instance,
	int preferredSlot
) {
	const int resultId = result->getId();
	const auto& bVertices = result->getBVertices();
	auto atSlot = [&](int slot) -> GraphVertex* {
		auto it = matchToGraph.find(to_string(instance) + "," + to_string(slot));
		if (it == matchToGraph.end()) {
			return nullptr;
		}
		int graphId = 0;
		int bIndex = 0;
		if (!parseIndexPair(it->second, graphId, bIndex)) {
			return nullptr;
		}
		if (graphId != resultId || bIndex < 0 || bIndex >= (int)bVertices.size()) {
			return nullptr;
		}
		return bVertices[bIndex];
	};
	if (auto* vertex = atSlot(preferredSlot)) {
		return vertex;
	}
	for (const auto& entry : matchToGraph) {
		int inst = 0;
		int slot = 0;
		if (!parseIndexPair(entry.first, inst, slot) || inst != instance) {
			continue;
		}
		if (auto* vertex = atSlot(slot)) {
			return vertex;
		}
	}
	return nullptr;
}

void setBVerticesFromTemplate(
	Graph* result,
	const unordered_map<string, string>& matchToGraph,
	const TemplateMatcher& matcher,
	int matchIndex,
	const vector<string>& boundaryIds
) {
	if (!result || boundaryIds.empty()) {
		return;
	}
	vector<GraphVertex*> ordered;
	ordered.reserve(boundaryIds.size());
	for (const string& boundaryId : boundaryIds) {
		int templateVertex = -1;
		for (int v = 0; v < (int)matcher.templateGraph.vertices.size(); v++) {
			if (matcher.templateGraph.vertices[v].boundaryId == boundaryId) {
				templateVertex = v;
				break;
			}
		}
		int instance = -1;
		int slot = -1;
		GraphVertex* stub = nullptr;
		if (templateVertex >= 0 && matcher.remainingStub(matchIndex, templateVertex, instance, slot)) {
			stub = liveStub(matchToGraph, result, instance, slot);
		}
		if (!stub) {
			throw runtime_error("buildGraphFromValues: no remaining stub for boundaryId " + boundaryId);
		}
		ordered.push_back(stub);
	}
	result->setBVertices(ordered);
}

GlueTrack glueVertices(
	GraphVertex* vertexA,
	GraphVertex* vertexB,
	Graph* graphA,
	Graph* graphB,
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
	// Copy before merge: graphA->merge clears graphB's bVertices vector.
	vector<GraphVertex*> savedBVerticesA(bVerticesA.begin(), bVerticesA.end());
	vector<GraphVertex*> savedBVerticesB(bVerticesB.begin(), bVerticesB.end());

	if (!loopGluing) {
		graphA->merge(graphB);
	}

	for (auto& halfBs : halfEdgesB) {
		for (auto* halfBSlot : halfBs) {
			if (halfBSlot) {
				halfBSlot->connectEdge(edgeA, halfBSlot->getEdgeIndex());
			}
		}
	}
	graphA->removeEdge(edgeB);

	for (size_t i = 0; i < halfEdgesA.size(); i++) {
		bool aOnBoundary = halfEdgesA[i][0] && halfEdgesA[i][0]->getVertex() == vertexA;
		auto* half0 = aOnBoundary ? halfEdgesB[i][0] : halfEdgesA[i][0];
		auto* half1 = aOnBoundary ? halfEdgesA[i][0] : halfEdgesB[i][0];
		if (half0 && half1) {
			GraphHalfEdge* halfToRemove = half0->getNext();
			glueHalfEdges(half0, half1, graphA);
			removeHalfEdge(halfToRemove, graphA);
		}
	}

	graphA->removeVertex(vertexA);
	graphA->removeVertex(vertexB);
	updateBoundaryVertices(graphA);

	const auto& newBVertices = graphA->getBVertices();
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
	unordered_map<FaceType*, vector<HalfEdgeFaceSlot>> halfEdgesByFaceType;
	vector<GraphVertex*> bVertices;
	GraphVertex* center = (new GraphVertex())->connectGraph(graph);
	center->setType(vType);

	for (size_t i = 0; i < halfEdgeTypes.size(); i++) {
		const auto& halfEdgeType = halfEdgeTypes[i];
		EdgeType* edgeType = halfEdgeType.edge;

		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(edgeType);

		auto* bVertex = (new GraphVertex())->connectGraph(graph);
		bVertex->setType(edgeVertexType());
		bVertices.push_back(bVertex);

		const auto& faceData = edgeType->getFaceData();
		for (size_t faceIndex = 0; faceIndex < faceData.size(); faceIndex++) {
			const auto& faceDatum = faceData[faceIndex];
			const auto& faceType = faceDatum.type;
			bool forward = !faceDatum.onRight;
			auto* half = (new GraphHalfEdge(forward))->connectGraph(graph);
			half->connectEdge(graphEdge, (int)faceIndex);

			// Each face has three half-edges:
			//   0. inwardHalf: intoVertex = true and it is going from a boundary vertex to the center node.
			//   1. outwardHalf: intoVertex = false and it is going from the center node to a boundary vertex.
			//   2. bonusHalf: Its vertex is a boundary vertex and it leads nowhere.
			bool intoVertex = !(faceDatum.onRight ^ halfEdgeType.isAtStart);
			GraphHalfEdge* bonusHalf = nullptr;
			if (intoVertex) {
				half->connectVertex(bVertex, -1);
			} else {
				half->connectVertex(center, -1);
				bonusHalf = (new GraphHalfEdge(false))->connectGraph(graph);
				bonusHalf->connectVertex(bVertex, -1);
				half->connectNext(bonusHalf);
			}

			// Add half-edge to halfEdgesByFaceType with angle and intoVertex.
			double angle = faceType->angle(halfEdgeType.dir);
			halfEdgesByFaceType[faceType].push_back({angle, intoVertex, half});
		}
	}
	// Sort half-edges by their face angle.
	for (auto& entry : halfEdgesByFaceType) {
		stable_sort(entry.second.begin(), entry.second.end(), compareHalfEdgeFaceSlots);
		orderSameAnglePairs(entry.second);
	}

	// For each inward facing half-edge, find it's outward facing partner and create a face.
	for (const auto& entry : halfEdgesByFaceType) {
		FaceType* faceType = entry.first;
		const auto& faceSlots = entry.second;
		const PartnerDirection direction = partnerDirection(faceSlots);
		for (size_t i = 0; i < faceSlots.size(); i++) {
			const auto& inwardSlot = faceSlots[i];
			if (!inwardSlot.intoVertex) {
				// Skip outward half-edges. They'll be paired with an inward half-edge later.
				continue;
			}
			auto* inwardHalf = inwardSlot.halfEdge;
			auto* outwardHalf = findOutwardPartner(faceSlots, i, direction);
			auto* bonusHalf = outwardHalf->getNext();

			inwardHalf->connectNext(outwardHalf);

			auto* face = (new GraphFace())->connectGraph(graph);
			face->setType(faceType);
			face->setOuterComponent(inwardHalf);

			inwardHalf->setFace(face);
			outwardHalf->setFace(face);
			bonusHalf->setFace(face);
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

Graph* getPrimitiveGraph(
	const GraphValues& graphValues,
	size_t index,
	const PrimitiveGraphs& graphs
) {
	const bool onBoundary = graphValues.vertexOnBoundary[index];
	const auto& primitives = onBoundary ? graphs.edgeGraphs : graphs.vertexGraphs;
	return primitives[graphValues.vertices[index]].get();
}

PrimitiveGraphs createPrimitiveGraphs(Primitives* primitives) {
	PrimitiveGraphs result;
	result.vertexGraphs.reserve(primitives->vertexTypes.size());
	for (auto* vType : primitives->vertexTypes) {
		result.vertexGraphs.push_back(unique_ptr<Graph>(createVertexGraph(vType)));
	}

	result.edgeGraphs.reserve(primitives->edgeTypes.size());
	for (auto* eType : primitives->edgeTypes) {
		result.edgeGraphs.push_back(unique_ptr<Graph>(createEdgeGraph(eType)));
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
	const PrimitiveGraphs& graphs,
	const TemplateMatcher& matcher,
	int matchIndex,
	const vector<string>& boundaryIds
) {
	if (graphValues.edges.empty() && graphValues.vertices.size() == 0) {
		// return Graph::createEmpty(primitives);
		return new Graph();
	}

	vector<unique_ptr<Graph>> instances;
	instances.reserve(graphValues.vertices.size());
	for (size_t i = 0; i < graphValues.vertices.size(); i++) {
		Graph* prototype = getPrimitiveGraph(graphValues, i, graphs);
		instances.push_back(unique_ptr<Graph>(prototype->copy()));
	}

	unordered_map<string, string> matchToGraph;
	unordered_map<string, string> graphToMatch;
	unordered_map<int, Graph*> graphMap;

	for (size_t i = 0; i < instances.size(); i++) {
		auto* graph = instances[i].get();
		const auto& bVertices = graph->getBVertices();
		for (size_t j = 0; j < bVertices.size(); j++) {
			string matchKey = to_string(i) + "," + to_string(j);
			string graphKey = to_string(graph->getId()) + "," + to_string(j);
			matchToGraph[matchKey] = graphKey;
			graphToMatch[graphKey] = matchKey;
		}
		graphMap[graph->getId()] = graph;
	}

	vector<array<int, 4>> edgeQueue = graphValues.edges;
	Graph* finalResult = nullptr;

	if (edgeQueue.empty()) {
		Graph* result = instances[0].release();
		setBVerticesFromTemplate(result, matchToGraph, matcher, matchIndex, boundaryIds);
		updateBoundaryHalfEdges(result);
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
			auto graphKeyA = matchToGraph[keyA];
			auto graphKeyB = matchToGraph[keyB];
			auto commaA = graphKeyA.find(',');
			auto commaB = graphKeyB.find(',');
			int graphIdA = stoi(graphKeyA.substr(0, commaA));
			int nBVertA = stoi(graphKeyA.substr(commaA + 1));
			int graphIdB = stoi(graphKeyB.substr(0, commaB));
			int nBVertB = stoi(graphKeyB.substr(commaB + 1));

			auto* graphA = graphMap[graphIdA];
			auto* graphB = graphMap[graphIdB];

			if (graphIdA == graphIdB) {
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

			auto outcome = copyAndGlue(*graphA, nBVertA, *graphB, nBVertB, graphIdA == graphIdB);
			auto& track = outcome.second;
			Graph* merged = outcome.first.get();
			int mergedId = merged->getId();

			for (size_t index = 0; index < track.aDest.size(); index++) {
				if (track.aDest[index] >= 0) {
					string oldMatchKey = graphToMatch[to_string(graphIdA) + "," + to_string(index)];
					string newGraphKey = to_string(mergedId) + "," + to_string(track.aDest[index]);
					matchToGraph[oldMatchKey] = newGraphKey;
					graphToMatch[newGraphKey] = oldMatchKey;
				}
			}
			if (graphIdA != graphIdB) {
				for (size_t index = 0; index < track.bDest.size(); index++) {
					if (track.bDest[index] >= 0) {
						string oldMatchKey = graphToMatch[to_string(graphIdB) + "," + to_string(index)];
						string newGraphKey = to_string(mergedId) + "," + to_string(track.bDest[index]);
						matchToGraph[oldMatchKey] = newGraphKey;
						graphToMatch[newGraphKey] = oldMatchKey;
					}
				}
				graphMap.erase(graphIdB);
			}

			finalResult = merged;
			graphMap[mergedId] = finalResult;
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
	setBVerticesFromTemplate(finalResult, matchToGraph, matcher, matchIndex, boundaryIds);
	updateBoundaryHalfEdges(finalResult);
	return releaseInstance(instances, finalResult);
}

bool loopsAreValid(Graph* graph) {
	bool hasOuterLoop = false;
	for (int i = 0; i < graph->getFaces().size(); i++) {
		auto* face = graph->getFaces()[i];
		if (face->isLoopy()) {
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

GraphFace* findOuterLoopFace(Graph* graph) {
	for (auto* face : graph->getFaces()) {
		if (face->getOuterComponent() && face->isLoopy() && face->computeTurns() == 1) {
			return face;
		}
	}
	return nullptr;
}

Graph* createFaceGraph(FaceType* face) {
	Graph* graph = new Graph();
	auto* graphFace = (new GraphFace())->connectGraph(graph);
	graphFace->setType(face);
	graphFace->setOuterComponent(nullptr);
	graph->setBHalfEdges({ nullptr });
	return graph;
}

// Add a boundary face to both graphs, if the filled graph has an outer loop.
void maybeAddBFace(Graph*& graph, Graph* filledGraph, bool addBFaces) {
	GraphFace* outerFace = findOuterLoopFace(filledGraph);
	if (outerFace) {
		delete graph;
		graph = createFaceGraph(outerFace->getType());
		if (addBFaces) {
			graph->setBFaces({graph->getFaces()[0]});
			filledGraph->setBFaces({ outerFace });
		}
	}
}

void alignBoundaryCycle(Graph* left, Graph* right);

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

		// TODO: The last parameter should probably be removed. It there because
		// we do not have any faces for 2D graphs to be attached to. In the future,
		// we should create a default ground plane for them.
		if (leftEmpty) {
			maybeAddBFace(leftGraph, rightGraph, grammar->isGrounded());
		} else if (rightEmpty) {
			maybeAddBFace(rightGraph, leftGraph, grammar->isGrounded());
		}
		if (!leftEmpty && !rightEmpty) {
			alignBoundaryCycle(leftGraph, rightGraph);
		}
		updateBoundaryHalfEdges(leftGraph);
		updateBoundaryHalfEdges(rightGraph);
		// If a graph is empty, it should go first.
		vector<Graph*> graphs = rightEmpty
			? vector<Graph*>{ rightGraph, leftGraph }
			: vector<Graph*>{ leftGraph, rightGraph };
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

struct BoundaryKey {
	int edgeTypeId = -1;
	bool forward = false;

	bool operator==(const BoundaryKey& other) const {
		return edgeTypeId == other.edgeTypeId && forward == other.forward;
	}

	bool operator<(const BoundaryKey& other) const {
		if (edgeTypeId != other.edgeTypeId) {
			return edgeTypeId < other.edgeTypeId;
		}
		return forward < other.forward;
	}
};

BoundaryKey stubKey(GraphVertex* vertex) {
	GraphHalfEdge* half = vertex ? vertex->interiorHalfEdge() : nullptr;
	GraphEdge* edge = vertex ? vertex->interiorEdge() : nullptr;
	if (!half || !edge || !edge->getType()) {
		return {};
	}
	return { edge->getType()->getId(), half->getForward() };
}

GraphHalfEdge* walkOneFace(GraphHalfEdge* half) {
	while (half->getNext()) {
		half = half->getNext();
	}
	return half->getPrev()->getTwin();
}

// Cycle order of the stubs GlueTrack already put in bVertices.
vector<GraphVertex*> boundaryCycle(Graph* graph) {
	const auto& bVertices = graph->getBVertices();
	if (bVertices.empty()) {
		return {};
	}
	GraphHalfEdge* start = bVertices[0]->interiorHalfEdge();
	if (!start) {
		return {};
	}
	vector<GraphVertex*> order;
	GraphHalfEdge* current = walkOneFace(start);
	order.push_back(current->getVertex());
	while (current != start) {
		current = walkOneFace(current);
		order.push_back(current->getVertex());
	}
	return order;
}

bool keysMatchShifted(
	const vector<BoundaryKey>& leftKeys,
	const vector<BoundaryKey>& rightKeys,
	int start
) {
	const int n = (int)leftKeys.size();
	for (int i = 0; i < n; i++) {
		if (!(leftKeys[i] == rightKeys[(i + start) % n])) {
			return false;
		}
	}
	return true;
}

int findCycleShift(const vector<BoundaryKey>& leftKeys, const vector<BoundaryKey>& rightKeys) {
	const int n = (int)leftKeys.size();
	for (int start = 0; start < n; start++) {
		if (keysMatchShifted(leftKeys, rightKeys, start)) {
			return start;
		}
	}
	return -1;
}

vector<BoundaryKey> cycleKeys(const vector<GraphVertex*>& order) {
	vector<BoundaryKey> keys;
	keys.reserve(order.size());
	for (auto* vertex : order) {
		keys.push_back(stubKey(vertex));
	}
	return keys;
}

void alignBoundaryCycle(Graph* left, Graph* right) {
	auto leftOrder = boundaryCycle(left);
	auto rightOrder = boundaryCycle(right);
	if (leftOrder.size() != rightOrder.size() || leftOrder.empty()) {
		return;
	}
	const int start = findCycleShift(cycleKeys(leftOrder), cycleKeys(rightOrder));
	if (start < 0) {
		return;
	}
	left->setBVertices(leftOrder);
	vector<GraphVertex*> rotated;
	rotated.reserve(rightOrder.size());
	for (size_t i = 0; i < rightOrder.size(); i++) {
		rotated.push_back(rightOrder[(i + start) % rightOrder.size()]);
	}
	right->setBVertices(rotated);
}

vector<string> collectBoundaryIds(const vector<TemplateMatcher>& matchers) {
	for (const auto& matcher : matchers) {
		vector<string> ids;
		for (const auto& vertex : matcher.templateGraph.vertices) {
			if (!vertex.boundaryId.empty()) {
				ids.push_back(vertex.boundaryId);
			}
		}
		if (!ids.empty()) {
			return ids;
		}
	}
	return {};
}

string boundaryTypeKey(Graph* graph) {
	auto keys = cycleKeys(boundaryCycle(graph));
	const int n = (int)keys.size();
	if (n == 0) {
		return {};
	}
	int best = 0;
	for (int start = 1; start < n; start++) {
		for (int i = 0; i < n; i++) {
			const auto& a = keys[(best + i) % n];
			const auto& b = keys[(start + i) % n];
			if (b < a) {
				best = start;
				break;
			}
			if (a < b) {
				break;
			}
		}
	}
	string key;
	for (int i = 0; i < n; i++) {
		if (i > 0) {
			key += ",";
		}
		const auto& k = keys[(best + i) % n];
		key += to_string(k.edgeTypeId);
		key += k.forward ? "F" : "B";
	}
	return key;
}

bool graphIsEmpty(Graph* graph) {
	return graph->getVertices().empty() && graph->getEdges().empty();
}

void RuleExporter::exportRules(
	GraphGrammar& grammar,
	const vector<TemplateMatcher>& matchers,
	const PrimitiveGraphs& primitiveGraphs
) {
	if (matchers.size() < 2) {
		return;
	}
	vector<string> boundaryIds = collectBoundaryIds(matchers);
	const int numGraphs = (int)matchers.size();
	vector<vector<unique_ptr<Graph>>> graphs(numGraphs);
	vector<vector<vector<int>>> vertexTypeIds(numGraphs);
	vector<vector<string>> typeKeys(numGraphs);

	for (int i = 0; i < numGraphs; i++) {
		for (int index = 0; index < (int)matchers[i].vertexValues.size(); index++) {
			auto graphValues = matchers[i].getGraphValues(index);
			auto graph = unique_ptr<Graph>(buildGraphFromValues(
				graphValues, primitiveGraphs, matchers[i], index, boundaryIds));
			if (!loopsAreValid(graph.get())) {
				continue;
			}
			auto ids = getVertexTypeIds(graph.get());
			string key = boundaryTypeKey(graph.get());
			bool duplicate = false;
			for (size_t j = 0; j < graphs[i].size(); j++) {
				if (typeKeys[i][j] != key) {
					continue;
				}
				if (isIsomorphic(graph.get(), ids, graphs[i][j].get(), vertexTypeIds[i][j])) {
					duplicate = true;
					break;
				}
			}
			if (!duplicate) {
				graphs[i].push_back(std::move(graph));
				vertexTypeIds[i].push_back(std::move(ids));
				typeKeys[i].push_back(std::move(key));
			}
		}
	}

	bool anyEmptyLeft = false;
	bool anyEmptyRight = false;
	for (const auto& left : graphs[0]) {
		if (graphIsEmpty(left.get())) {
			anyEmptyLeft = true;
		}
	}
	for (const auto& right : graphs[1]) {
		if (graphIsEmpty(right.get())) {
			anyEmptyRight = true;
		}
	}
	if (anyEmptyLeft || anyEmptyRight) {
		for (const auto& left : graphs[0]) {
			for (const auto& right : graphs[1]) {
				if (graphIsEmpty(left.get()) == graphIsEmpty(right.get())) {
					continue;
				}
				exportRule(&grammar, left->copy(), right->copy());
			}
		}
		return;
	}

	map<string, vector<Graph*>> leftByKey;
	map<string, vector<Graph*>> rightByKey;
	for (size_t i = 0; i < graphs[0].size(); i++) {
		if (!typeKeys[0][i].empty()) {
			leftByKey[typeKeys[0][i]].push_back(graphs[0][i].get());
		}
	}
	for (size_t i = 0; i < graphs[1].size(); i++) {
		if (!typeKeys[1][i].empty()) {
			rightByKey[typeKeys[1][i]].push_back(graphs[1][i].get());
		}
	}
	for (const auto& entry : leftByKey) {
		auto found = rightByKey.find(entry.first);
		if (found == rightByKey.end()) {
			continue;
		}
		for (auto* left : entry.second) {
			for (auto* right : found->second) {
				exportRule(&grammar, left->copy(), right->copy());
			}
		}
	}
}
