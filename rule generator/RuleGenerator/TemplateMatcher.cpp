#include "pch.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"
#include <set>
#include <iostream>
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
	counter = 0;
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

bool TemplateMatcher::orientedNormalWalk(
	int vIndex,
	int stateIndex,
	int& fromConn,
	int& toConn
) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	const int n = (int)vertex.connections.size();
	if (!vertex.spliced || stateIndex < 0 || stateIndex >= numEdgeStates) {
		return false;
	}

	int normalConns[2] = { -1, -1 };
	int normalCount = 0;
	for (int i = 0; i < n; i++) {
		if (!templateGraph.edges[vertex.connections[i]].spliced) {
			if (normalCount < 2) {
				normalConns[normalCount] = i;
			}
			normalCount++;
		}
	}
	if (normalCount != 2) {
		return false;
	}

	int neigh[2];
	for (int k = 0; k < 2; k++) {
		int e = vertex.connections[normalConns[k]];
		const auto& ends = eConnections[e];
		neigh[k] = (ends[0] == vIndex) ? ends[1] : ends[0];
	}

	fromConn = normalConns[0];
	toConn = normalConns[1];
	if (neigh[0] > neigh[1]) {
		fromConn = normalConns[1];
		toConn = normalConns[0];
	} else if (neigh[0] == neigh[1] && normalConns[0] > normalConns[1]) {
		fromConn = normalConns[1];
		toConn = normalConns[0];
	}

	if (edgeStates[stateIndex].getHalfEdgeIndex() == 1) {
		std::swap(fromConn, toConn);
	}
	return true;
}

bool TemplateMatcher::selectSplicedFaceSide(
	int vIndex,
	int stateIndex,
	int connIndex,
	bool& onRightOut
) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	const int n = (int)vertex.connections.size();
	if (!vertex.spliced || connIndex < 0 || connIndex >= n) {
		return false;
	}

	const int edgeIndex = vertex.connections[connIndex];
	if (!templateGraph.edges[edgeIndex].spliced) {
		return false;
	}

	int fromConn = -1;
	int toConn = -1;
	if (!orientedNormalWalk(vIndex, stateIndex, fromConn, toConn)) {
		return false;
	}

	// CCW side of the oriented walk → left (onRight=false); CW side → right.
	const bool onCcw = spliceOnCcwArc(fromConn, toConn, connIndex, n);
	onRightOut = !onCcw;
	return true;
}

string TemplateMatcher::connectionIdAt(int vIndex, int stateIndex, int connIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	if (!vertex.spliced) {
		return getState(vIndex, stateIndex).GetConnectionId(connIndex);
	}

	// Spliced vertex: one edge state spans both normal half-edges.
	// Start-side advertises id; end-side advertises oppositeId — so a
	// boundary S and boundary E can both attach to the same mid-edge state.
	int fromConn = -1;
	int toConn = -1;
	if (!orientedNormalWalk(vIndex, stateIndex, fromConn, toConn)) {
		return getState(vIndex, stateIndex).GetConnectionId(connIndex);
	}
	const EdgeState& edgeState = edgeStates[stateIndex];
	if (connIndex == fromConn) {
		return edgeState.getId();
	}
	if (connIndex == toConn) {
		return edgeState.getOppositeId();
	}
	// Spliced half-edge — callers should use faceIdForSplicedConnection instead.
	return getState(vIndex, stateIndex).GetConnectionId(connIndex);
}

int TemplateMatcher::connectionIndexAt(int vIndex, int stateIndex, int connIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	if (!vertex.spliced) {
		return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
	}

	const int edgeIndex = vertex.connections[connIndex];
	if (templateGraph.edges[edgeIndex].spliced) {
		// Fixed slot in createSplicedVertexGraph: bVertices[2] = splice.
		return 2;
	}

	int fromConn = -1;
	int toConn = -1;
	if (!orientedNormalWalk(vIndex, stateIndex, fromConn, toConn)) {
		return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
	}
	const EdgeState& edgeState = edgeStates[stateIndex];
	// bVertices[0] = segment start, bVertices[1] = segment end.
	if (connIndex == fromConn) {
		return edgeState.getHalfEdgeIndex();
	}
	if (connIndex == toConn) {
		return 1 - edgeState.getHalfEdgeIndex();
	}
	return edgeState.getHalfEdgeIndex();
}

bool TemplateMatcher::splicedFaceOnRight(
	int vIndex,
	int stateIndex,
	int connIndex,
	bool& onRightOut
) const {
	return selectSplicedFaceSide(vIndex, stateIndex, connIndex, onRightOut);
}

string TemplateMatcher::faceIdForSplicedConnection(int vIndex, int stateIndex, int connIndex) const {
	bool onRight = false;
	if (!selectSplicedFaceSide(vIndex, stateIndex, connIndex, onRight)) {
		return "";
	}

	const EdgeState& edgeState = edgeStates[stateIndex];
	EdgeType* eType = edgeTypes[edgeState.getTypeValue()];
	for (const auto& faceDatum : eType->getFaceData()) {
		if (faceDatum.onRight == onRight) {
			return faceTypeId(faceDatum.type);
		}
	}
	return "";
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
		counter++;
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

			if (splicedEdge) {
				for (int j = 0; j < numStates; j++) {
					if (rejectionStep[updateIndex][j] == -1) {
						string faceId = faceIdForSplicedConnection(updateIndex, j, i);
						if (faceId.empty()) {
							reject(updateIndex, j);
						}
						else {
							neighborIds.insert(faceId);
						}
					}
				}
			}
			else {
				for (int j = 0; j < numStates; j++) {
					if (rejectionStep[updateIndex][j] == -1) {
						string connectionId = connectionIdAt(updateIndex, j, i);
						string neighborId = HalfEdgeType::oppositeId(connectionId);
						neighborIds.insert(neighborId);
					}
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
					string connectionId = splicedEdge
						? faceIdForSplicedConnection(neighbor, j, cIndex)
						: connectionIdAt(neighbor, j, cIndex);
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
		graphValues.vertices.push_back(getState(j, vertexValue[j]).getTypeValue());

		const bool spliced = templateVertex.spliced;
		graphValues.vertexSpliced.push_back(spliced);
		graphValues.vertexOnBoundary.push_back(!spliced && !templateVertex.boundaryId.empty());

		bool onRight = false;
		bool spliceIsAtStart = true;
		if (spliced) {
			int spliceConn = -1;
			int spliceEdge = -1;
			for (int c = 0; c < (int)templateVertex.connections.size(); c++) {
				int e = templateVertex.connections[c];
				if (templateGraph.edges[e].spliced) {
					spliceConn = c;
					spliceEdge = e;
					break;
				}
			}
			if (spliceConn >= 0) {
				splicedFaceOnRight(j, vertexValue[j], spliceConn, onRight);
				spliceIsAtStart = (templateGraph.edges[spliceEdge].start == j);
			}
		}
		graphValues.spliceOnRight.push_back(onRight);
		graphValues.spliceIsAtStart.push_back(spliceIsAtStart);
	}

	// Resolve where each boundary tip survives after gluing, so exporters can
	// order bVertices by shared boundaryId across both sides of a rule.
	const int nTemplate = (int)vertexValue.size();
	graphValues.tipInstance.assign(nTemplate, -1);
	graphValues.tipSlot.assign(nTemplate, -1);
	for (int j = 0; j < nTemplate; j++) {
		const auto& templateVertex = templateGraph.vertices[j];
		if (templateVertex.boundaryId.empty() || templateVertex.spliced) {
			continue;
		}
		for (int c = 0; c < (int)templateVertex.connections.size(); c++) {
			const int e = templateVertex.connections[c];
			const auto& ends = eConnections[e];
			const int neighbor = (ends[0] == j) ? ends[1] : ends[0];
			if (neighbor < 0 || neighbor >= nTemplate) {
				continue;
			}
			if (templateGraph.vertices[neighbor].spliced) {
				// Boundary edge-graph glues away the state slot; the opposite end remains.
				const int gluedSlot = connectionIndexAt(j, vertexValue[j], c);
				graphValues.tipInstance[j] = templateToMatch[j];
				graphValues.tipSlot[j] = 1 - gluedSlot;
				break;
			}
			if (templateGraph.vertices[neighbor].boundaryId.empty() &&
				!templateGraph.vertices[neighbor].spliced) {
				// Interior vertex primitive owns the boundary spoke.
				const int neighborConn = ConnectionIndex(neighbor, e, -1);
				graphValues.tipInstance[j] = templateToMatch[neighbor];
				graphValues.tipSlot[j] = connectionIndexAt(
					neighbor, vertexValue[neighbor], neighborConn
				);
				break;
			}
		}
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
			edge[edgeIndex++] = connectionIndexAt(vIdx, vertexValue[vIdx], cIndex);
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
