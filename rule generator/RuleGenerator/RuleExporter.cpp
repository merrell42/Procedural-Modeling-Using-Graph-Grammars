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

	for (size_t i = 0; i < halfEdgeTypes.size(); i++) {
		const auto& halfEdgeType = halfEdgeTypes[i];
		EdgeType* edgeType = halfEdgeType.edge;
		bool isAtStart = halfEdgeType.isAtStart;

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
			bool forward = !faceDatum.onRight;
			auto* half = (new GraphHalfEdge(forward))->connectGraph(graph);
			half->connectEdge(graphEdge, (int)faceIndex);

			int position = faceDatum.onRight ^ isAtStart;
			int faceId = halfEdgeType.faceIds[(int)faceIndex];
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
			throw runtime_error("createVertexGraph: missing half-edge for a face");
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
	const PrimitiveGraphs& graphs
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
	updateBoundaryHalfEdges(finalResult);
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
		// TODO: Handle rules with splices in them.
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
	auto primitiveGraphs = createPrimitiveGraphs(primitives);
	for (const auto& group : groups) {
		const int numGraphs = (int)group.graphIndices.size();
		vector<vector<unique_ptr<Graph>>> graphs(numGraphs);
		vector<vector<vector<int>>> vertexTypeIds(numGraphs);

		for (int i = 0; i < numGraphs; i++) {
			for (int index : group.graphIndices[i]) {
				auto graphValues = matchers[i].getGraphValues(index);
				auto graphA = unique_ptr<Graph>(buildGraphFromValues(graphValues, primitiveGraphs));
				auto vertexTypeIdsA = getVertexTypeIds(graphA.get());
				if (loopsAreValid(graphA.get()) &&
					!isDuplicateGraph(graphA.get(), vertexTypeIdsA, graphs[i], vertexTypeIds[i])) {
					graphs[i].push_back(std::move(graphA));
					vertexTypeIds[i].push_back(std::move(vertexTypeIdsA));
				}
			}
		}

		// Assumes there are only two graphs in the template set.
		for (const auto& left : graphs[0]) {
			for (const auto& right : graphs[1]) {
				exportRule(&grammar, left->copy(), right->copy());
			}
		}
	}
}
