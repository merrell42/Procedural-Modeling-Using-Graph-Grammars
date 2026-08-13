#include "pch.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"
#include <set>
#include <iostream>
#include <optional>
#include <utility>

using namespace std;

namespace {

bool spliceOnCcwArc(int fromConn, int toConn, int spliceConn, int n) {
	for (int i = (fromConn + 1) % n; i != toConn; i = (i + 1) % n) {
		if (i == spliceConn) {
			return true;
		}
	}
	return false;
}

}  // namespace

TemplateMatcher::TemplateMatcher(
	TemplateGraph templateGraph_,
	vector<VertexType*> vTypes,
	vector<EdgeType*> eTypes
) : templateGraph(templateGraph_), edgeTypes(std::move(eTypes)) {
	for (int i = 0; i < (int)vTypes.size(); i++) {
		VertexType* vType = vTypes[i];
		for (int j = 0; j < (int)vType->getHalfEdgeTypes().size(); j++) {
			vertexStates.push_back(VertexState(vType, i, j));
		}
	}
	for (int i = 0; i < (int)edgeTypes.size(); i++) {
		EdgeType* eType = edgeTypes[i];
		string id = eType->getRuleGeneratorId();
		edgeStates.push_back(EdgeState(id + "S", i, 0));
		edgeStates.push_back(EdgeState(id + "E", i, 1));
	}
	numVertexStates = (int)vertexStates.size();
	numEdgeStates = (int)edgeStates.size();
	numTemplateVertices = (int)templateGraph.vertices.size();

	// Build eConnections from explicit edge list.
	for (const auto& edge : templateGraph.edges) {
		eConnections.push_back({edge.start, edge.end});
	}

	// -1 means not rejected.
	inQueue = new bool[numTemplateVertices];
	for (int i = 0; i < numTemplateVertices; i++) {
		inQueue[i] = false;
		vector<int> rejectStepOneVertex;
		const auto& templateVertex = templateGraph.vertices[i];
		const int numConnections = (int)templateVertex.connections.size();
		const bool edgeValued = usesEdgeState(i);
		int numStates = numStatesAtVertex(i);
		for (int j = 0; j < numStates; j++) {
			int rejectAt = -1;
			// Immediately reject any state that does not have the correct number of edges.
			if (!edgeValued && vertexStates[j].getType()->getHalfEdgeTypes().size() != numConnections) {
				rejectAt = 0;
			}
			rejectStepOneVertex.push_back(rejectAt);
		}
		rejectionStep.push_back(rejectStepOneVertex);
	}
	vIndex = 0;
}

bool TemplateMatcher::usesEdgeState(int vIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	return !vertex.boundaryId.empty() || vertex.spliced;
}

int TemplateMatcher::numStatesAtVertex(int vIndex) const {
	return usesEdgeState(vIndex) ? numEdgeStates : numVertexStates;
}

const State& TemplateMatcher::getState(int vIndex, int stateIndex) const {
	return usesEdgeState(vIndex)
		? static_cast<const State&>(edgeStates[stateIndex])
		: static_cast<const State&>(vertexStates[stateIndex]);
}

string TemplateMatcher::faceTypeId(FaceType* type) {
	if (!type) {
		return "";
	}
	return "F:" + to_string(reinterpret_cast<uintptr_t>(type));
}

optional<TemplateMatcher::SpliceLayout> TemplateMatcher::spliceLayout(
	int vIndex,
	int stateIndex
) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	const int n = (int)vertex.connections.size();
	if (!vertex.spliced || stateIndex < 0 || stateIndex >= numEdgeStates) {
		return nullopt;
	}

	int normalConns[2] = { -1, -1 };
	int spliceConn = -1;
	int spliceEdge = -1;
	int normalCount = 0;
	for (int i = 0; i < n; i++) {
		int e = vertex.connections[i];
		if (templateGraph.edges[e].spliced) {
			if (spliceConn < 0) {
				spliceConn = i;
				spliceEdge = e;
			}
		} else {
			if (normalCount < 2) {
				normalConns[normalCount] = i;
			}
			normalCount++;
		}
	}
	if (normalCount != 2 || spliceConn < 0) {
		return nullopt;
	}

	int neigh[2];
	for (int k = 0; k < 2; k++) {
		int e = vertex.connections[normalConns[k]];
		const auto& ends = eConnections[e];
		neigh[k] = (ends[0] == vIndex) ? ends[1] : ends[0];
	}

	SpliceLayout layout;
	layout.fromConn = normalConns[0];
	layout.toConn = normalConns[1];
	if (neigh[0] > neigh[1] || (neigh[0] == neigh[1] && normalConns[0] > normalConns[1])) {
		layout.fromConn = normalConns[1];
		layout.toConn = normalConns[0];
	}

	const EdgeState& edgeState = edgeStates[stateIndex];
	if (edgeState.GetConnectionIndex(layout.fromConn) == 1) {
		std::swap(layout.fromConn, layout.toConn);
	}

	layout.spliceConn = spliceConn;
	layout.spliceEdge = spliceEdge;
	// CCW side of the oriented walk → left (onRight=false); CW side → right.
	layout.onRight = !spliceOnCcwArc(layout.fromConn, layout.toConn, spliceConn, n);
	layout.startId = edgeState.getName();
	layout.endId = edgeState.GetConnectionId(layout.toConn);
	layout.startB = edgeState.GetConnectionIndex(layout.fromConn);
	layout.endB = 1 - layout.startB;

	EdgeType* eType = edgeTypes[edgeState.getTypeValue()];
	for (const auto& faceDatum : eType->getFaceData()) {
		if (faceDatum.onRight == layout.onRight) {
			layout.faceId = faceTypeId(faceDatum.type);
			break;
		}
	}
	return layout;
}

string TemplateMatcher::advertisedId(int vIndex, int stateIndex, int connIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	if (connIndex >= 0 && connIndex < (int)vertex.connections.size() &&
		templateGraph.edges[vertex.connections[connIndex]].spliced) {
		auto layout = spliceLayout(vIndex, stateIndex);
		if (!layout || connIndex != layout->spliceConn) {
			return "";
		}
		return layout->faceId;
	}

	if (!vertex.spliced) {
		return getState(vIndex, stateIndex).GetConnectionId(connIndex);
	}

	// Spliced vertex: one edge state spans both normal half-edges.
	// Start-side advertises id; end-side advertises oppositeId — so a
	// boundary S and boundary E can both attach to the same mid-edge state.
	auto layout = spliceLayout(vIndex, stateIndex);
	if (!layout) {
		return getState(vIndex, stateIndex).GetConnectionId(connIndex);
	}
	if (connIndex == layout->fromConn) {
		return layout->startId;
	}
	if (connIndex == layout->toConn) {
		return layout->endId;
	}
	return getState(vIndex, stateIndex).GetConnectionId(connIndex);
}

int TemplateMatcher::advertisedBIndex(int vIndex, int stateIndex, int connIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	if (!vertex.spliced) {
		return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
	}

	auto layout = spliceLayout(vIndex, stateIndex);
	if (!layout) {
		return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
	}
	if (connIndex == layout->spliceConn) {
		return SpliceLayout::spliceB;
	}
	if (connIndex == layout->fromConn) {
		return layout->startB;
	}
	if (connIndex == layout->toConn) {
		return layout->endB;
	}
	return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
}

void TemplateMatcher::match() {
	if (numTemplateVertices == 0) {
		vertexValues.push_back({});
		return;
	}
	Decision decison0(0);
	int numStates = numStatesAtVertex(0);
	for (int j = 0; j < numStates; j++) {
		decison0.choices.push_back(j);
	}
	decisions.push_back(decison0);
	while (decisions.size() > 0) {
		applyDecision();
		bool success = propagate();
		if (success) {
			bool found = findNextChoice();
			if (!found) {
				undoLastDecision();
			}
		}
		else {
			updateQueue.clear();
			for (int j = 0; j < numTemplateVertices; j++) {
				inQueue[j] = false;
			}
			undoLastDecision();
		}
	}
}

void TemplateMatcher::reject(int pos, int type) {
	rejectionStep[pos][type] = (int)decisions.size();
	if (!inQueue[pos]) {
		inQueue[pos] = true;
		updateQueue.push_back(pos);
	}
}

void TemplateMatcher::applyDecision() {
	Decision decision = decisions.back();
	int choice = decision.choices[0];

	int numStates = numStatesAtVertex(vIndex);
	for (int i = 0; i < numStates; i++) {
		if (i != choice && rejectionStep[vIndex][i] < 0) {
			reject(vIndex, i);
		}
	}
}

int TemplateMatcher::ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex) const {
	auto nConnections = templateGraph.vertices[vertexIndex].connections;
	for (int i = 0; i < (int)nConnections.size(); i++) {
		if (nConnections[i] == edgeIndex && i != excludeIndex) {
			return i;
		}
	}
	cout << "Connection not found." << endl;
	return -1;
}

bool TemplateMatcher::propagate() {
	while (updateQueue.size() > 0) {
		int updateIndex = updateQueue[0];
		updateQueue.erase(updateQueue.begin());
		inQueue[updateIndex] = false;
		auto vConnections = templateGraph.vertices[updateIndex].connections;
		set<string> neighborIds;
		for (int i = 0; i < (int)vConnections.size(); i++) {
			neighborIds.clear();
			int vConnection = vConnections[i];
			const bool splicedEdge = templateGraph.edges[vConnection].spliced;
			int numStates = numStatesAtVertex(updateIndex);

			for (int j = 0; j < numStates; j++) {
				if (rejectionStep[updateIndex][j] != -1) {
					continue;
				}
				string id = advertisedId(updateIndex, j, i);
				if (splicedEdge) {
					if (id.empty()) {
						reject(updateIndex, j);
					} else {
						neighborIds.insert(id);
					}
				} else {
					neighborIds.insert(HalfEdgeType::oppositeId(id));
				}
			}

			auto edgeConnections = eConnections[vConnection];
			int neighbor = (edgeConnections[0] == updateIndex) ? edgeConnections[1] : edgeConnections[0];
			int excludeIndex = -1;
			// When the neighbor is the same as the current vertex, we have the same edge repeated
			// twice. Exclude the current edge. Switch to the other one.
			if (updateIndex == neighbor) {
				excludeIndex = i;
			}

			bool hasMatch = false;
			int cIndex = ConnectionIndex(neighbor, vConnection, excludeIndex);
			int neighborStates = numStatesAtVertex(neighbor);
			for (int j = 0; j < neighborStates; j++) {
				if (rejectionStep[neighbor][j] == -1) {
					string connectionId = advertisedId(neighbor, j, cIndex);
					auto it = neighborIds.find(connectionId);
					if (it != neighborIds.end()) {
						hasMatch = true;
					}
					else {
						reject(neighbor, j);
					}
				}
			}
			if (!hasMatch) {
				return false;
			}
		}
	}
	return true;
}

vector<int> TemplateMatcher::findChoices() {
	vector<int> choices;
	int numStates = numStatesAtVertex(vIndex);
	for (int i = 0; i < numStates; i++) {
		if (rejectionStep[vIndex][i] < 0) {
			choices.push_back(i);
		}
	}
	return choices;
}

void TemplateMatcher::acceptMatch() {
	vector<int> newGraphStates;
	for (int i = 0; i < numTemplateVertices; i++) {
		int numStates = numStatesAtVertex(i);
		for (int j = 0; j < numStates; j++) {
			if (rejectionStep[i][j] == -1) {
				newGraphStates.push_back(j);
			}
		}
	}
	vertexValues.push_back(newGraphStates);
}

bool TemplateMatcher::findNextChoice() {
	while (true) {
		vIndex++;
		if (vIndex >= numTemplateVertices) {
			acceptMatch();
			return false;
		}
		auto choices = findChoices();
		if (choices.size() > 1) {
			Decision decision(vIndex);
			decision.choices = choices;
			decisions.push_back(decision);
			return true;
		}
	}
}

void TemplateMatcher::undoLastDecision() {
	int n = (int)decisions.size();
	if (n == 0) {
		return;
	}
	for (int i = 0; i < numTemplateVertices; i++) {
		int numStates = numStatesAtVertex(i);
		for (int j = 0; j < numStates; j++) {
			if (rejectionStep[i][j] >= n) {
				rejectionStep[i][j] = -1;
			}
		}
	}
	vIndex = decisions.back().vIndex;
	decisions.back().choices.erase(decisions.back().choices.begin());
	if (decisions.back().choices.size() == 0) {
		decisions.pop_back();
		undoLastDecision();
	}
}

GraphValues TemplateMatcher::getGraphValues(int graphIndex) const {
	GraphValues graphValues;

	auto vertexValue = vertexValues[graphIndex];
	vector<int> templateToMatch(templateGraph.vertices.size(), -1);

	for (int j = 0; j < (int)vertexValue.size(); j++) {
		const auto& templateVertex = templateGraph.vertices[j];
		templateToMatch[j] = (int)graphValues.vertices.size();

		GraphValues::Site site;
		site.typeValue = getState(j, vertexValue[j]).getTypeValue();
		if (templateVertex.spliced) {
			site.kind = GraphValues::Site::Spliced;
			if (auto layout = spliceLayout(j, vertexValue[j])) {
				site.spliceOnRight = layout->onRight;
				site.spliceIsAtStart = (templateGraph.edges[layout->spliceEdge].start == j);
			}
		} else if (!templateVertex.boundaryId.empty()) {
			site.kind = GraphValues::Site::Boundary;
		}
		graphValues.vertices.push_back(site);
	}

	vector<array<int, 4>> normalEdges;
	vector<array<int, 4>> splicedEdges;
	for (int j = 0; j < (int)eConnections.size(); j++) {
		auto vIndices = eConnections[j];
		if (vIndices.size() != 2) {
			cout << "Edge should have 2 vertices." << endl;
			continue;
		}
		if (templateToMatch[vIndices[0]] < 0 || templateToMatch[vIndices[1]] < 0) {
			continue;
		}

		const bool spliced0 = templateGraph.vertices[vIndices[0]].spliced;
		const bool spliced1 = templateGraph.vertices[vIndices[1]].spliced;
		const bool onBoundary0 = !templateGraph.vertices[vIndices[0]].boundaryId.empty();
		const bool onBoundary1 = !templateGraph.vertices[vIndices[1]].boundaryId.empty();

		// Interior↔boundary half-edges are represented inside vertex primitives, not as glue edges.
		// Spliced sites are their own primitives and must glue to neighbors (including boundary).
		if (!spliced0 && !spliced1 && onBoundary0 != onBoundary1) {
			continue;
		}

		array<int, 4> edge{};
		size_t edgeIndex = 0;
		for (int vIdx : vIndices) {
			int cIndex = ConnectionIndex(vIdx, j, -1);
			edge[edgeIndex++] = templateToMatch[vIdx];
			edge[edgeIndex++] = advertisedBIndex(vIdx, vertexValue[vIdx], cIndex);
		}
		if (templateGraph.edges[j].spliced) {
			splicedEdges.push_back(edge);
		} else {
			normalEdges.push_back(edge);
		}
	}
	// Glue splice links first so mid-edge sites merge before boundary attachments.
	graphValues.edges = std::move(splicedEdges);
	graphValues.edges.insert(graphValues.edges.end(), normalEdges.begin(), normalEdges.end());
	return graphValues;
}
