#include "pch.h"
#include "TemplateMatcher.h"
#include <set>
#include <iostream>

using namespace std;

namespace {
constexpr PrimitiveType Vertex = PrimitiveType::Vertex;
constexpr PrimitiveType Edge = PrimitiveType::Edge;
constexpr PrimitiveType Spliced = PrimitiveType::Spliced;

bool isGlueablePair(PrimitiveType a, PrimitiveType b) {
	if (a == Spliced || b == Spliced) {
		if (a == Spliced && b == Spliced) {
			return false;
		}
		PrimitiveType other = a == Spliced ? b : a;
		return other == Vertex || other == Edge;
	}
	return a == b;
}
}  // namespace

TemplateMatcher::TemplateMatcher(
	TemplateGraph templateGraph_,
	vector<VertexType*> vTypes,
	vector<EdgeType*> eTypes,
	vector<FaceType*> faceTypes_
) : templateGraph(templateGraph_), edgeTypes(eTypes), faceTypes(faceTypes_) {
	counter = 0;
	for (int i = 0; i < vTypes.size(); i++) {
		VertexType* vType = vTypes[i];
		for (int j = 0; j < vType->getHalfEdgeTypes().size(); j++) {
			vertexStates.push_back(VertexState(vType, i, j));
		}
	}
	for (int i = 0; i < eTypes.size(); i++) {
		EdgeType* eType = eTypes[i];
		if (eType->getSpliced()) {
			continue;
		}
		string id = eType->getRuleGeneratorId();
		edgeStates.push_back(EdgeState(eType, id + "S", i, 0));
		edgeStates.push_back(EdgeState(eType, id + "E", i, 1));
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
		vector <int> rejectStepOneVertex;
		const auto& templateVertex = templateGraph.vertices[i];
		const int numConnections = (int)templateVertex.connections.size();
		const bool usesEdgeStatesI = usesEdgeStates(i);
		int numStates = numStatesAtVertex(i);
		for (int j = 0; j < numStates; j++) {
			int rejectAt = -1;
			// Immediately reject any state that does not have the correct number of edges.
			if (!usesEdgeStatesI && vertexStates[j].getType()->getHalfEdgeTypes().size() != numConnections) {
				rejectAt = 0;
			}
			rejectStepOneVertex.push_back(rejectAt);
		}
		rejectionStep.push_back(rejectStepOneVertex);
	}
	vIndex = 0;
}

bool TemplateMatcher::usesEdgeStates(int vIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	return !vertex.boundaryId.empty() || vertex.spliced;
}

bool TemplateMatcher::isSplicedVertex(int vIndex) const {
	return templateGraph.vertices[vIndex].spliced;
}

bool TemplateMatcher::isBoundaryVertex(int vIndex) const {
	return !templateGraph.vertices[vIndex].boundaryId.empty();
}

int TemplateMatcher::numStatesAtVertex(int vIndex) {
	return usesEdgeStates(vIndex) ? numEdgeStates : numVertexStates;
}

const State& TemplateMatcher::getState(int vIndex, int stateIndex) const {
	return usesEdgeStates(vIndex)
		? static_cast<const State&>(edgeStates[stateIndex])
		: static_cast<const State&>(vertexStates[stateIndex]);
}

int TemplateMatcher::neighborAcrossEdge(int vIndex, int edgeIndex) const {
	const auto& edgeConnections = eConnections[edgeIndex];
	return (edgeConnections[0] == vIndex) ? edgeConnections[1] : edgeConnections[0];
}

string TemplateMatcher::getConnectionId(int vIndex, int stateIndex, int connIndex) const {
	const auto& vertex = templateGraph.vertices[vIndex];
	int edgeIndex = vertex.connections[connIndex];

	const bool isSplicedVertexI = templateGraph.vertices[vIndex].spliced;
	if (!isSplicedVertexI || templateGraph.edges[edgeIndex].spliced) {
		return getState(vIndex, stateIndex).GetConnectionId(connIndex);
	}

	const EdgeState& edgeState = edgeStates[stateIndex];
	const EdgeType* eType = edgeState.getType();
	Vec3 dir = eType->getDir();
	if (edgeState.getHalfEdgeIndex() != 0) {
		dir = dir * -1.0f;
	}

	const auto& vPos = vertex;
	int neighbor = neighborAcrossEdge(vIndex, edgeIndex);
	const auto& nPos = templateGraph.vertices[neighbor];
	double dx = nPos.posX - vPos.posX;
	double dy = nPos.posY - vPos.posY;
	double dot = dx * dir.getX() + dy * dir.getY();

	// Toward the edge start: emit oppositeId so the neighbor can match with id.
	return dot < 0 ? edgeState.getOppositeId() : edgeState.getId();
}

FaceType* TemplateMatcher::faceTypeAtSpliceConnection(int vIndex, int stateIndex, int connIndex) const {
	const EdgeState& edgeState = edgeStates[stateIndex];
	const EdgeType* eType = edgeState.getType();
	const auto& faceData = eType->getFaceData();

	Vec3 dir = eType->getDir();
	if (edgeState.getHalfEdgeIndex() != 0) {
		dir = dir * -1.0f;
	}

	const auto& vertex = templateGraph.vertices[vIndex];
	int edgeIndex = vertex.connections[connIndex];
	int spliceNeighbor = neighborAcrossEdge(vIndex, edgeIndex);
	const auto& nPos = templateGraph.vertices[spliceNeighbor];
	double sx = nPos.posX - vertex.posX;
	double sy = nPos.posY - vertex.posY;
	double cross = dir.getX() * sy - dir.getY() * sx;

	for (const auto& fd : faceData) {
		if ((cross < 0) == fd.onRight) {
			return fd.type;
		}
	}
	return faceData.empty() ? nullptr : faceData[0].type;
}

int TemplateMatcher::spliceConnectionIndex(int vIndex) const {
	const auto& connections = templateGraph.vertices[vIndex].connections;
	for (int i = 0; i < (int)connections.size(); i++) {
		if (templateGraph.edges[connections[i]].spliced) {
			return i;
		}
	}
	return -1;
}

bool TemplateMatcher::spliceOnRight(int vIndex, int stateIndex) const {
	int connIndex = spliceConnectionIndex(vIndex);
	if (connIndex < 0) {
		return false;
	}
	const EdgeState& edgeState = edgeStates[stateIndex];
	const EdgeType* eType = edgeState.getType();
	Vec3 dir = eType->getDir();
	if (edgeState.getHalfEdgeIndex() != 0) {
		dir.scale(-1);
	}
	const auto& vertex = templateGraph.vertices[vIndex];
	int edgeIndex = vertex.connections[connIndex];
	int spliceNeighbor = neighborAcrossEdge(vIndex, edgeIndex);
	const auto& nPos = templateGraph.vertices[spliceNeighbor];
	double sx = nPos.posX - vertex.posX;
	double sy = nPos.posY - vertex.posY;
	double cross = dir.getX() * sy - dir.getY() * sx;
	return cross < 0;
}

int TemplateMatcher::splicedBoundaryVertexIndex(int vIndex, int stateIndex, int connIndex) const {
	int edgeIndex = templateGraph.vertices[vIndex].connections[connIndex];
	bool onRight = spliceOnRight(vIndex, stateIndex);

	if (templateGraph.edges[edgeIndex].spliced) {
		return onRight ? 1 : 2;
	}

	const EdgeState& edgeState = edgeStates[stateIndex];
	Vec3 dir = edgeState.getType()->getDir();
	if (edgeState.getHalfEdgeIndex() != 0) {
		dir.scale(-1);
	}
	const auto& vertex = templateGraph.vertices[vIndex];
	int neighbor = neighborAcrossEdge(vIndex, edgeIndex);
	const auto& nPos = templateGraph.vertices[neighbor];
	double dx = nPos.posX - vertex.posX;
	double dy = nPos.posY - vertex.posY;
	double dot = dx * dir.getX() + dy * dir.getY();
	bool atStart = dot < 0;

	if (onRight) {
		return atStart ? 0 : 2;
	}
	return atStart ? 0 : 1;
}

int TemplateMatcher::boundaryVertexIndex(int vIndex, int stateIndex, int templateEdgeIndex, int connIndex) const {
	if (isSplicedVertex(vIndex)) {
		return splicedBoundaryVertexIndex(vIndex, stateIndex, connIndex);
	}
	return getState(vIndex, stateIndex).GetConnectionIndex(connIndex);
}

int TemplateMatcher::faceTypeIndex(FaceType* faceType) const {
	if (!faceType) {
		return -1;
	}
	for (int i = 0; i < (int)faceTypes.size(); i++) {
		if (faceTypes[i] == faceType) {
			return i;
		}
	}
	return -1;
}

int TemplateMatcher::splicedGraphIndex(int edgeTypeIndex, bool onRight) const {
	if (edgeTypeIndex < 0 || edgeTypeIndex >= (int)edgeTypes.size() || edgeTypes[edgeTypeIndex]->getSpliced()) {
		return -1;
	}
	int nonSplicedIndex = 0;
	for (int i = 0; i < edgeTypeIndex; i++) {
		if (!edgeTypes[i]->getSpliced()) {
			nonSplicedIndex++;
		}
	}
	return 2 * nonSplicedIndex + (onRight ? 1 : 0);
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
	for (int i = 0; i < nConnections.size(); i++) {
		if (nConnections[i] == edgeIndex && i != excludeIndex) {
			return i;
		}
	}
	cout << "Connection not found." << endl;
	return -1;
}

bool TemplateMatcher::propagateSplicedEdge(
	int updateIndex,
	int neighbor,
	int connIndex,
	int neighborConnIndex
) {
	set<FaceType*> neighborFaceTypes;
	int numStates = numStatesAtVertex(updateIndex);
	for (int j = 0; j < numStates; j++) {
		if (rejectionStep[updateIndex][j] == -1) {
			FaceType* faceType = faceTypeAtSpliceConnection(updateIndex, j, connIndex);
			if (faceType) {
				neighborFaceTypes.insert(faceType);
			}
		}
	}

	bool hasMatch = false;
	int neighborStates = numStatesAtVertex(neighbor);
	for (int j = 0; j < neighborStates; j++) {
		if (rejectionStep[neighbor][j] == -1) {
			FaceType* faceType = faceTypeAtSpliceConnection(neighbor, j, neighborConnIndex);
			if (faceType && neighborFaceTypes.count(faceType)) {
				hasMatch = true;
			} else {
				reject(neighbor, j);
			}
		}
	}
	return hasMatch;
}

bool TemplateMatcher::propagateEdge(
	int updateIndex,
	int neighbor,
	int connIndex,
	int neighborConnIndex
) {
	set<string> neighborIds;
	int numStates = numStatesAtVertex(updateIndex);
	for (int j = 0; j < numStates; j++) {
		if (rejectionStep[updateIndex][j] == -1) {
			string connectionId = getConnectionId(updateIndex, j, connIndex);
			string neighborId = HalfEdgeType::oppositeId(connectionId);
			neighborIds.insert(neighborId);
		}
	}

	bool hasMatch = false;
	int neighborStates = numStatesAtVertex(neighbor);
	for (int j = 0; j < neighborStates; j++) {
		if (rejectionStep[neighbor][j] == -1) {
			string connectionId = getConnectionId(neighbor, j, neighborConnIndex);
			if (neighborIds.count(connectionId)) {
				hasMatch = true;
			} else {
				reject(neighbor, j);
			}
		}
	}
	return hasMatch;
}

bool TemplateMatcher::propagate() {
	while (updateQueue.size() > 0) {
		int updateIndex = updateQueue[0];
		updateQueue.erase(updateQueue.begin());
		inQueue[updateIndex] = false;
		auto vConnections = templateGraph.vertices[updateIndex].connections;
		for (int i = 0; i < vConnections.size(); i++) {
			int vConnection = vConnections[i];
			auto edgeConnections = eConnections[vConnection];
			int neighbor = (edgeConnections[0] == updateIndex) ? edgeConnections[1] : edgeConnections[0];
			int excludeIndex = -1;
			// When the neighbor is the same as the current vertex, we have the same edge repeated
			// twice. Exclude the current edge. Switch to the other one.
			if (updateIndex == neighbor) {
				excludeIndex = i;
			}

			int cIndex = ConnectionIndex(neighbor, vConnection, excludeIndex);
			bool hasMatch = templateGraph.edges[vConnection].spliced
				? propagateSplicedEdge(updateIndex, neighbor, i, cIndex)
				: propagateEdge(updateIndex, neighbor, i, cIndex);

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
		const int numChoices = (int)choices.size();
		if (numChoices == 0) {
			return false;
		}
		if (numChoices > 1) {
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

	// Add the vertices.
	auto vertexValue = vertexValues[graphIndex];
	vector<int> templateToMatch(templateGraph.vertices.size(), -1);
	vector<PrimitiveType> primitiveTypes(templateGraph.vertices.size(), Vertex);

	for (int j = 0; j < (int)vertexValue.size(); j++) {
		const auto& templateVertex = templateGraph.vertices[j];
		int stateIndex = vertexValue[j];

		if (templateVertex.spliced) {
			const EdgeState& edgeState = edgeStates[stateIndex];
			int spliceConn = spliceConnectionIndex(j);
			bool onRight = spliceOnRight(j, stateIndex);
			int splicedIndex = splicedGraphIndex(edgeState.getTypeValue(), onRight);
			if (splicedIndex < 0) {
				templateToMatch[j] = -1;
				continue;
			}
			templateToMatch[j] = (int)graphValues.vertices.size();
			graphValues.vertices.push_back(splicedIndex);
			graphValues.primitiveType.push_back(Spliced);
			FaceType* spliceFace = faceTypeAtSpliceConnection(j, stateIndex, spliceConn);
			graphValues.spliceFaceTypeIndex.push_back(faceTypeIndex(spliceFace));
			primitiveTypes[j] = Spliced;
		} else if (!templateVertex.boundaryId.empty()) {
			templateToMatch[j] = (int)graphValues.vertices.size();
			graphValues.vertices.push_back(getState(j, stateIndex).getTypeValue());
			graphValues.primitiveType.push_back(Edge);
			graphValues.spliceFaceTypeIndex.push_back(-1);
			primitiveTypes[j] = Edge;
		} else {
			templateToMatch[j] = (int)graphValues.vertices.size();
			graphValues.vertices.push_back(getState(j, stateIndex).getTypeValue());
			graphValues.primitiveType.push_back(Vertex);
			graphValues.spliceFaceTypeIndex.push_back(-1);
			primitiveTypes[j] = Vertex;
		}
	}

	for (int j = 0; j < (int)eConnections.size(); j++) {
		auto vIndices = eConnections[j];
		if (vIndices.size() != 2) {
			cout << "Edge should have 2 vertices." << endl;
			continue;
		}

		int v0 = vIndices[0];
		int v1 = vIndices[1];
		if (templateToMatch[v0] < 0 || templateToMatch[v1] < 0) {
			continue;
		}
		PrimitiveType type0 = primitiveTypes[v0];
		PrimitiveType type1 = primitiveTypes[v1];

		if (templateGraph.edges[j].spliced) {
			int cIndex0 = ConnectionIndex(v0, j, -1);
			int cIndex1 = ConnectionIndex(v1, j, -1);
			graphValues.edges.push_back({
				templateToMatch[v0],
				boundaryVertexIndex(v0, vertexValue[v0], j, cIndex0),
				templateToMatch[v1],
				boundaryVertexIndex(v1, vertexValue[v1], j, cIndex1),
			});
			continue;
		}

		if (!isGlueablePair(type0, type1)) {
			continue;
		}

		array<int, 4> edge{};
		size_t edgeIndex = 0;
		for (int vIndex : vIndices) {
			int cIndex = ConnectionIndex(vIndex, j, -1);
			edge[edgeIndex++] = templateToMatch[vIndex];
			edge[edgeIndex++] = boundaryVertexIndex(vIndex, vertexValue[vIndex], j, cIndex);
		}
		graphValues.edges.push_back(edge);
	}
	return graphValues;
}

