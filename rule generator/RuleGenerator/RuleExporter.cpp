#include "pch.h"
#include "RuleExporter.h"
#include "isIsomorphic.h"

#include "../../cpp_version/primitives/vertex_type.h"
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

VertexType* splicedVertexType() {
	static VertexType* type = new VertexType();
	return type;
}

void removeGluedBoundaryVertices(Graph* graph, GraphVertex* vertexA, GraphVertex* vertexB) {
	vector<GraphVertex*> bVertices;
	for (auto* vertex : graph->getBVertices()) {
		if (vertex && vertex != vertexA && vertex != vertexB) {
			bVertices.push_back(vertex);
		}
	}
	graph->setBVertices(bVertices);
}

void refreshBoundaryVertices(Graph* graph) {
	vector<GraphVertex*> bVertices;
	for (auto* vertex : graph->getVertices()) {
		if (vertex && vertex->getType() == edgeVertexType()) {
			bVertices.push_back(vertex);
		}
	}
	graph->setBVertices(bVertices);
}

Graph* createGraphWithConnections(
	VertexType* centerType,
	const vector<HalfEdgeType>& connections
) {
	auto* graph = new Graph();
	const size_t connectionCount = connections.size();
	unordered_map<int, FaceBuildInfo> faceInfos;
	vector<GraphVertex*> bVertices;
	GraphVertex* center = nullptr;

	vector<vector<int>> connectionFaceIds(connectionCount);
	for (size_t connIndex = 0; connIndex < connectionCount; connIndex++) {
		vector<int> faceIds = { (int)connIndex, (int)((connIndex + 1) % connectionCount) };
		if (!connections[connIndex].isAtStart) {
			reverse(faceIds.begin(), faceIds.end());
		}
		connectionFaceIds[connIndex] = std::move(faceIds);
	}

	for (size_t connIndex = 0; connIndex < connectionCount; connIndex++) {
		const auto& connection = connections[connIndex];
		EdgeType* edgeType = connection.edge;
		bool isAtStart = connection.isAtStart;

		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(edgeType);

		auto* bVertex = (new GraphVertex())->connectGraph(graph);
		bVertex->setType(edgeVertexType());
		bVertices.push_back(bVertex);

		if (!center) {
			center = (new GraphVertex())->connectGraph(graph);
			center->setType(centerType);
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
				throw runtime_error("createGraphWithConnections: multiple faces with the same ID within a primitive");
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
			throw runtime_error("createGraphWithConnections: missing half-edge for face");
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

GraphHalfEdge* firstHalfEdge(GraphVertex* vertex) {
	for (auto* half : vertex->getHalfEdges()) {
		if (half) {
			return half;
		}
	}
	return nullptr;
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
		for (auto* half : graph->getHalfEdges()) {
			if (half && half->getFace() == face1) {
				half->setFace(face0);
			}
		}
		face0->mergeInto(face1);
		graph->removeFace(face1);
		face = face0;
	}
	if (face) {
		face->replaceHalfEdge(half0, replacement);
		face->replaceHalfEdge(half1, replacement);
	}

	for (auto* half : graph->getHalfEdges()) {
		if (!half || half == half0 || half == half1 || half == replacement) {
			continue;
		}
		if (half->getPrev() == half0 || half->getPrev() == half1) {
			half->setPrev(replacement);
		}
		if (half->getNext() == half0 || half->getNext() == half1) {
			half->connectNext(replacement);
		}
	}

	for (auto* vertex : graph->getVertices()) {
		if (!vertex) {
			continue;
		}
		const auto& vertexHalfEdges = vertex->getHalfEdges();
		for (size_t i = 0; i < vertexHalfEdges.size(); i++) {
			if (vertexHalfEdges[i] == half0) {
				vertex->setHalfEdge(replacement, (int)i);
			} else if (vertexHalfEdges[i] == half1) {
				vertex->setHalfEdge(nullptr, (int)i);
			}
		}
	}

	for (auto* edge : graph->getEdges()) {
		if (!edge) {
			continue;
		}
		auto& edgeHalfEdges = const_cast<vector<vector<GraphHalfEdge*>>&>(edge->getHalfEdges());
		for (auto& slot : edgeHalfEdges) {
			for (auto*& half : slot) {
				if (half == half0 || half == half1) {
					half = replacement;
				}
			}
		}
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

	auto graphHasSplicedEdge = [](Graph* graph) {
		for (auto* edge : graph->getEdges()) {
			if (edge && edge->getType() && edge->getType()->getSpliced()) {
				return true;
			}
		}
		return false;
	};
	bool involvesSplicedEdge = graphHasSplicedEdge(netA) || graphHasSplicedEdge(netB);

	for (size_t i = 0; i < halfEdgesA.size(); i++) {
		bool aOnBoundary = halfEdgesA[i][0] && halfEdgesA[i][0]->getVertex() == vertexA;
		auto* half0 = aOnBoundary ? halfEdgesB[i][0] : halfEdgesA[i][0];
		auto* half1 = aOnBoundary ? halfEdgesA[i][0] : halfEdgesB[i][0];
		if (half0 && half1) {
			GraphHalfEdge* halfToRemove = half0->getNext();
			glueHalfEdges(half0, half1, netA);
			if (!involvesSplicedEdge && halfToRemove) {
				removeHalfEdge(halfToRemove, netA);
			}
		}
	}

	unordered_set<GraphVertex*> bVertexSet(
		netA->getBVertices().begin(),
		netA->getBVertices().end()
	);
	vector<GraphVertex*> interiorVertices;
	for (auto* vertex : netA->getVertices()) {
		if (vertex && vertex != vertexA && vertex != vertexB && !bVertexSet.count(vertex)) {
			interiorVertices.push_back(vertex);
		}
	}
	if (!interiorVertices.empty() && involvesSplicedEdge) {
		GraphVertex* targetInterior = interiorVertices[0];
		for (size_t i = 1; i < interiorVertices.size(); i++) {
			GraphVertex* extraInterior = interiorVertices[i];
			for (auto* half : netA->getHalfEdges()) {
				if (half && half->getVertex() == extraInterior) {
					half->connectVertex(targetInterior, -1);
				}
			}
			netA->removeVertex(extraInterior);
		}
		for (auto* half : netA->getHalfEdges()) {
			if (half && (half->getVertex() == vertexA || half->getVertex() == vertexB)) {
				half->connectVertex(targetInterior, -1);
			}
		}
	}

	netA->removeVertex(vertexA);
	netA->removeVertex(vertexB);
	if (involvesSplicedEdge) {
		removeGluedBoundaryVertices(netA, vertexA, vertexB);
	} else {
		refreshBoundaryVertices(netA);
	}

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
	return createGraphWithConnections(vType, vType->getHalfEdgeTypes());
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

Graph* createSplicedVertexGraph(
	EdgeType* segmentEdgeType,
	bool spliceOnRight,
	EdgeType* splicedEdgeType
) {
	// Connections in CCW order around the center; bVertex index matches connection order.
	vector<HalfEdgeType> connections = spliceOnRight
		? vector<HalfEdgeType>{
			HalfEdgeType(segmentEdgeType, true),
			HalfEdgeType(splicedEdgeType, true),
			HalfEdgeType(segmentEdgeType, false),
		}
		: vector<HalfEdgeType>{
			HalfEdgeType(segmentEdgeType, true),
			HalfEdgeType(segmentEdgeType, false),
			HalfEdgeType(splicedEdgeType, true),
		};
	return createGraphWithConnections(splicedVertexType(), connections);
}

void applySplicedEdgeType(Graph* graph, EdgeType* splicedEdgeType) {
	for (auto* edge : graph->getEdges()) {
		if (edge && edge->getType() && edge->getType()->getSpliced()) {
			edge->setType(splicedEdgeType);
			return;
		}
	}
	throw runtime_error("applySplicedEdgeType: spliced edge not found");
}

Graph* getPrimitiveGraph(
	const GraphValues& graphValues,
	size_t index,
	const PrimitiveGraphs& graphs
) {
	switch (graphValues.primitiveType[index]) {
	case PrimitiveType::Edge:
		return graphs.edgeGraphs[graphValues.vertices[index]].get();
	case PrimitiveType::Spliced:
		return graphs.splicedGraphs[graphValues.vertices[index]].get();
	default:
		return graphs.vertexGraphs[graphValues.vertices[index]].get();
	}
}

PrimitiveGraphs createPrimitiveGraphs(Primitives* primitives) {
	PrimitiveGraphs result;
	result.vertexGraphs.reserve(primitives->vertexTypes.size());
	for (VertexType* vType : primitives->vertexTypes) {
		result.vertexGraphs.push_back(unique_ptr<Graph>(createVertexGraph(vType)));
	}

	result.edgeGraphs.reserve(primitives->edgeTypes.size());
	for (EdgeType* eType : primitives->edgeTypes) {
		result.edgeGraphs.push_back(unique_ptr<Graph>(createEdgeGraph(eType)));
	}

	if (primitives->faceTypes.empty()) {
		return result;
	}
	EdgeType* placeholderSplicedEdgeType = primitives->getSplicedEdgeType(primitives->faceTypes[0]);
	for (EdgeType* eType : primitives->edgeTypes) {
		if (eType->getSpliced()) {
			continue;
		}
		result.splicedGraphs.push_back(unique_ptr<Graph>(
			createSplicedVertexGraph(eType, false, placeholderSplicedEdgeType)
		));
		result.splicedGraphs.push_back(unique_ptr<Graph>(
			createSplicedVertexGraph(eType, true, placeholderSplicedEdgeType)
		));
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

vector<array<int, 4>> orderGlueEdges(const vector<array<int, 4>>& edges, size_t instanceCount) {
	vector<int> parent(instanceCount);
	for (size_t i = 0; i < instanceCount; i++) {
		parent[i] = (int)i;
	}
	auto findRoot = [&](int x) {
		while (parent[x] != x) {
			parent[x] = parent[parent[x]];
			x = parent[x];
		}
		return x;
	};
	auto unite = [&](int a, int b) {
		a = findRoot(a);
		b = findRoot(b);
		if (a != b) {
			parent[b] = a;
		}
	};

	vector<array<int, 4>> mergeEdges;
	vector<array<int, 4>> loopEdges;
	for (const auto& edge : edges) {
		int instanceA = edge[0];
		int instanceB = edge[2];
		if (findRoot(instanceA) != findRoot(instanceB)) {
			mergeEdges.push_back(edge);
			unite(instanceA, instanceB);
		} else {
			loopEdges.push_back(edge);
		}
	}
	mergeEdges.insert(mergeEdges.end(), loopEdges.begin(), loopEdges.end());
	return mergeEdges;
}

Graph* buildGraphFromValues(
	const GraphValues& graphValues,
	const PrimitiveGraphs& graphs,
	Primitives* primitives
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
		if (graphValues.primitiveType[i] == PrimitiveType::Spliced) {
			int faceIndex = graphValues.spliceFaceTypeIndex[i];
			if (faceIndex < 0 || faceIndex >= (int)primitives->faceTypes.size()) {
				throw runtime_error("buildGraphFromValues: invalid splice face type index");
			}
			EdgeType* splicedEdgeType = primitives->getSplicedEdgeType(
				primitives->faceTypes[faceIndex]
			);
			applySplicedEdgeType(instances.back().get(), splicedEdgeType);
		}
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

	vector<array<int, 4>> edgeQueue = orderGlueEdges(
		graphValues.edges,
		graphValues.vertices.size()
	);
	Graph* finalResult = nullptr;

	if (edgeQueue.empty()) {
		Graph* result = instances[0].release();
		updateBoundaryHalfEdges(result);
		return result;
	}

	auto applyGlue = [&](const array<int, 4>& edge, bool allowLoop) -> bool {
		int vertexA = edge[0];
		int matchBoundaryVertexIndexA = edge[1];
		int vertexB = edge[2];
		int matchBoundaryVertexIndexB = edge[3];

		string keyA = to_string(vertexA) + "," + to_string(matchBoundaryVertexIndexA);
		string keyB = to_string(vertexB) + "," + to_string(matchBoundaryVertexIndexB);
		auto netKeyA = matchToNetwork[keyA];
		auto netKeyB = matchToNetwork[keyB];
		if (netKeyA.empty() || netKeyB.empty()) {
			return false;
		}
		auto commaA = netKeyA.find(',');
		auto commaB = netKeyB.find(',');
		int netA = stoi(netKeyA.substr(0, commaA));
		int boundaryVertexIndexA = stoi(netKeyA.substr(commaA + 1));
		int netB = stoi(netKeyB.substr(0, commaB));
		int boundaryVertexIndexB = stoi(netKeyB.substr(commaB + 1));

		auto graphAIt = networkMap.find(netA);
		auto graphBIt = networkMap.find(netB);
		if (graphAIt == networkMap.end() || graphBIt == networkMap.end()) {
			return false;
		}
		auto* graphA = graphAIt->second;
		auto* graphB = graphBIt->second;

		if (netA == netB) {
			if (!allowLoop) {
				return false;
			}
		}

		auto outcome = copyAndGlue(*graphA, boundaryVertexIndexA, *graphB, boundaryVertexIndexB, netA == netB);
		auto& track = outcome.second;
		Graph* merged = outcome.first.get();
		int mergedId = merged->getId();

		for (size_t index = 0; index < track.aDest.size(); index++) {
			if (track.aDest[index] >= 0) {
				string oldNetKey = to_string(netA) + "," + to_string(index);
				auto matchIt = networkToMatch.find(oldNetKey);
				if (matchIt == networkToMatch.end()) {
					continue;
				}
				string newNetKey = to_string(mergedId) + "," + to_string(track.aDest[index]);
				matchToNetwork[matchIt->second] = newNetKey;
				networkToMatch[newNetKey] = matchIt->second;
			}
		}
		if (netA != netB) {
			for (size_t index = 0; index < track.bDest.size(); index++) {
				if (track.bDest[index] >= 0) {
					string oldNetKey = to_string(netB) + "," + to_string(index);
					auto matchIt = networkToMatch.find(oldNetKey);
					if (matchIt == networkToMatch.end()) {
						continue;
					}
					string newNetKey = to_string(mergedId) + "," + to_string(track.bDest[index]);
					matchToNetwork[matchIt->second] = newNetKey;
					networkToMatch[newNetKey] = matchIt->second;
				}
			}
			networkMap.erase(netB);
		}

		finalResult = merged;
		networkMap[mergedId] = finalResult;
		instances.push_back(std::move(outcome.first));
		return true;
	};

	while (!edgeQueue.empty()) {
		bool progress = false;
		for (size_t i = 0; i < edgeQueue.size(); i++) {
			if (applyGlue(edgeQueue[i], false)) {
				edgeQueue.erase(edgeQueue.begin() + (int)i);
				progress = true;
				break;
			}
		}
		if (!progress) {
			for (size_t i = 0; i < edgeQueue.size(); i++) {
				if (applyGlue(edgeQueue[i], true)) {
					edgeQueue.erase(edgeQueue.begin() + (int)i);
					progress = true;
					break;
				}
			}
		}
		if (!progress) {
			throw runtime_error("buildGraphFromValues: cannot glue graph");
		}
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
				auto graphA = unique_ptr<Graph>(buildGraphFromValues(graphValues, primitiveGraphs, primitives));
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
