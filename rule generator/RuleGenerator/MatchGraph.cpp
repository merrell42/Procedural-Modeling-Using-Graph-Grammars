#include "pch.h"
#include "MatchGraph.h"

#include "../../cpp_version/util/util.h"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace {

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

vector<GraphVertex*> getConnectors(Graph* graph) {
	return graph->getBVertices();
}

void refreshBVertices(Graph* graph) {
	vector<GraphVertex*> connectors;
	for (auto* v : graph->getVertices()) {
		if (v && v->getType() == edgeVertexType()) {
			connectors.push_back(v);
		}
	}
	graph->setBVertices(connectors);
}

vector<pair<GraphVertex*, GraphVertex*>> findLoopables(Graph* graph) {
	vector<pair<GraphVertex*, GraphVertex*>> loopables;
	auto connectors = getConnectors(graph);
	unordered_set<GraphVertex*> connSet(connectors.begin(), connectors.end());
	for (auto* face : graph->getFaces()) {
		auto outer = face->getOuterHalfEdges();
		for (size_t i = 0; i < outer.size(); i++) {
			auto* v0 = outer[i]->getVertex();
			auto* v1 = outer[(i + 1) % outer.size()]->getVertex();
			if (v0 && v1 && v0 != v1 && connSet.count(v0) && connSet.count(v1)) {
				loopables.push_back({ v0, v1 });
			}
		}
	}
	return loopables;
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
	if (face0 && face1 && face0 != face1) {
		face0->mergeInto(face1);
		graph->removeFace(face1);
	}
	if (face0) {
		face0->replaceHalfEdge(half0, replacement);
		face0->replaceHalfEdge(half1, replacement);
	}

	graph->removeHalfEdge(half0);
	half0->disconnectEdge();
	graph->removeHalfEdge(half1);
	half1->disconnectEdge();

	return replacement;
}

GlueTrack glueConnectors(
	GraphVertex* connA,
	GraphVertex* connB,
	Graph* netA,
	Graph* netB,
	const vector<GraphVertex*>& connectorsA,
	const vector<GraphVertex*>& connectorsB,
	bool loopGluing
) {
	auto* halfA = firstHalfEdge(connA);
	auto* halfB = firstHalfEdge(connB);
	if (!halfA || !halfB) {
		throw runtime_error("glueConnectors: connector has no half-edge");
	}

	auto* edgeA = halfA->getEdge() ? halfA->getEdge() : halfA->getPrev()->getEdge();
	auto* edgeB = halfB->getEdge() ? halfB->getEdge() : halfB->getPrev()->getEdge();

	auto halfEdgesA = edgeA->getHalfEdges();
	auto halfEdgesB = edgeB->getHalfEdges();

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
		bool aOnBoundary = halfEdgesA[i][0] && halfEdgesA[i][0]->getVertex() == connA;
		auto* half0 = aOnBoundary ? halfEdgesB[i][0] : halfEdgesA[i][0];
		auto* half1 = aOnBoundary ? halfEdgesA[i][0] : halfEdgesB[i][0];
		if (half0 && half1) {
			glueHalfEdges(half0, half1, netA);
			if (half0->getNext()) {
				netA->removeHalfEdge(half0->getNext());
			}
		}
	}

	netA->removeVertex(connA);
	netA->removeVertex(connB);
	refreshBVertices(netA);

	vector<GraphVertex*> newConnectors = getConnectors(netA);
	GlueTrack track;
	for (auto* a : connectorsA) {
		track.aDest.push_back(indexOf(newConnectors, a));
	}
	for (auto* b : connectorsB) {
		track.bDest.push_back(indexOf(newConnectors, b));
	}
	return track;
}

pair<unique_ptr<Graph>, GlueTrack> copyAndGlue(
	Graph& graphA,
	int connectorIndexA,
	Graph& graphB,
	int connectorIndexB,
	bool loopGluing
) {
	auto connectorsA = getConnectors(&graphA);
	auto connectorsB = getConnectors(&graphB);

	if (loopGluing) {
		auto copyA = unique_ptr<Graph>(graphA.copy());
		auto track = glueConnectors(
			getConnectors(copyA.get())[connectorIndexA],
			getConnectors(copyA.get())[connectorIndexB],
			copyA.get(),
			copyA.get(),
			getConnectors(copyA.get()),
			getConnectors(copyA.get()),
			true
		);
		return { std::move(copyA), track };
	}

	auto copyA = unique_ptr<Graph>(graphA.copy());
	auto copyB = unique_ptr<Graph>(graphB.copy());
	auto track = glueConnectors(
		getConnectors(copyA.get())[connectorIndexA],
		getConnectors(copyB.get())[connectorIndexB],
		copyA.get(),
		copyB.get(),
		getConnectors(copyA.get()),
		getConnectors(copyB.get()),
		false
	);
	return { std::move(copyA), track };
}

Graph* createVertexGraph(VertexType* vType) {
	auto* graph = new Graph();
	const auto& halfEdgeTypes = vType->getHalfEdgeTypes();
	unordered_map<FaceType*, FaceBuildInfo> faceInfos;
	vector<GraphVertex*> connectors;
	GraphVertex* center = nullptr;

	for (const auto& connection : halfEdgeTypes) {
		EdgeType* edgeType = connection.edge;
		bool isAtStart = connection.isAtStart;

		auto* graphEdge = (new GraphEdge())->connectGraph(graph);
		graphEdge->setType(edgeType);

		auto* connector = (new GraphVertex())->connectGraph(graph);
		connector->setType(edgeVertexType());
		connectors.push_back(connector);

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
			auto& faceInfo = faceInfos[faceDatum.type];
			faceInfo.type = faceDatum.type;
			faceInfo.hEdges[position] = half;

			if (position == 0) {
				half->connectVertex(connector, -1);
			} else {
				half->connectVertex(center, -1);
				auto* bonusHalf = (new GraphHalfEdge(false))->connectGraph(graph);
				bonusHalf->connectVertex(connector, -1);
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
	}

	graph->setBVertices(connectors);
	return graph;
}

}  // namespace

Match matchFromGraphValues(const GraphValues& values) {
	Match match;
	match.vertices = values.vertexValues;
	for (size_t i = 0; i + 3 < values.edgeValues.size(); i += 4) {
		match.edges.push_back({
			values.edgeValues[i],
			values.edgeValues[i + 1],
			values.edgeValues[i + 2],
			values.edgeValues[i + 3],
		});
	}
	return match;
}

vector<unique_ptr<Graph>> createVertexTypeGraphs(Primitives* primitives) {
	vector<unique_ptr<Graph>> graphs;
	graphs.reserve(primitives->vertexTypes.size());
	for (auto* vType : primitives->vertexTypes) {
		graphs.push_back(unique_ptr<Graph>(createVertexGraph(vType)));
	}
	return graphs;
}

vector<vector<int>> connectionOrdersFor(Primitives* primitives) {
	vector<vector<int>> orders;
	orders.reserve(primitives->vertexTypes.size());
	for (auto* vType : primitives->vertexTypes) {
		vector<int> order;
		order.reserve(vType->getHalfEdgeTypes().size());
		for (size_t i = 0; i < vType->getHalfEdgeTypes().size(); i++) {
			order.push_back((int)i);
		}
		orders.push_back(std::move(order));
	}
	return orders;
}

Graph* glueMatch(
	const Match& match,
	const vector<unique_ptr<Graph>>& vertexTypeGraphs,
	const vector<vector<int>>& connectionOrders
) {
	if (match.edges.empty() && match.vertices.size() == 1) {
		return vertexTypeGraphs[match.vertices[0]]->copy();
	}

	vector<unique_ptr<Graph>> instances;
	instances.reserve(match.vertices.size());
	for (int typeIndex : match.vertices) {
		instances.push_back(unique_ptr<Graph>(vertexTypeGraphs[typeIndex]->copy()));
	}

	unordered_map<string, string> matchToNetwork;
	unordered_map<string, string> networkToMatch;
	unordered_map<int, Graph*> networkMap;

	for (size_t i = 0; i < instances.size(); i++) {
		auto* graph = instances[i].get();
		auto connectors = getConnectors(graph);
		for (size_t j = 0; j < connectors.size(); j++) {
			string matchKey = to_string(i) + "," + to_string(j);
			string netKey = to_string(graph->getId()) + "," + to_string(j);
			matchToNetwork[matchKey] = netKey;
			networkToMatch[netKey] = matchKey;
		}
		networkMap[graph->getId()] = graph;
	}

	vector<array<int, 4>> edgeQueue = match.edges;
	Graph* finalResult = nullptr;

	while (!edgeQueue.empty()) {
		vector<array<int, 4>> nextQueue;
		for (const auto& edge : edgeQueue) {
			int vertexA = edge[0];
			int connectA = connectionOrders[match.vertices[vertexA]][edge[1]];
			int vertexB = edge[2];
			int connectB = connectionOrders[match.vertices[vertexB]][edge[3]];

			string keyA = to_string(vertexA) + "," + to_string(connectA);
			string keyB = to_string(vertexB) + "," + to_string(connectB);
			auto netKeyA = matchToNetwork[keyA];
			auto netKeyB = matchToNetwork[keyB];
			auto commaA = netKeyA.find(',');
			auto commaB = netKeyB.find(',');
			int netA = stoi(netKeyA.substr(0, commaA));
			int nConA = stoi(netKeyA.substr(commaA + 1));
			int netB = stoi(netKeyB.substr(0, commaB));
			int nConB = stoi(netKeyB.substr(commaB + 1));

			auto* graphA = networkMap[netA];
			auto* graphB = networkMap[netB];

			if (netA == netB) {
				auto loopables = findLoopables(graphA);
				bool canLoop = false;
				auto* connectorA = getConnectors(graphA)[nConA];
				auto* connectorB = getConnectors(graphA)[nConB];
				for (const auto& loopable : loopables) {
					if ((loopable.first == connectorA && loopable.second == connectorB) ||
						(loopable.second == connectorA && loopable.first == connectorB)) {
						canLoop = true;
						break;
					}
				}
				if (!canLoop) {
					nextQueue.push_back(edge);
					continue;
				}
			}

			auto outcome = copyAndGlue(*graphA, nConA, *graphB, nConB, netA == netB);
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
			throw runtime_error("glueMatch: cannot glue match");
		}
		edgeQueue = std::move(nextQueue);
	}

	if (!finalResult) {
		throw runtime_error("glueMatch: no result");
	}
	return finalResult->copy();
}

Graph* buildGraphFromValues(
	const GraphValues& values,
	const vector<unique_ptr<Graph>>& vertexTypeGraphs,
	const vector<vector<int>>& connectionOrders
) {
	Match match = matchFromGraphValues(values);
	return glueMatch(match, vertexTypeGraphs, connectionOrders);
}
