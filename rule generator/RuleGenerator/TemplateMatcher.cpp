#include "pch.h"
#include "TemplateMatcher.h"
#include <set>
#include <iostream>

using namespace std;

TemplateMatcher::TemplateMatcher(
	TemplateGraph templateGraph_,
	vector<VertexType*> vTypes,
	vector<EdgeType*> eTypes
) : templateGraph(templateGraph_) {
	counter = 0;
	for (int i = 0; i < vTypes.size(); i++) {
		VertexType* vType = vTypes[i];
		for (int j = 0; j < vType->getHalfEdgeTypes().size(); j++) {
			vertexStates.push_back(VertexState(vType, i, j));
		}
	}
	for (int i = 0; i < eTypes.size(); i++) {
		EdgeType* eType = eTypes[i];
		string id = eType->getRuleGeneratorId();
		edgeStates.push_back(EdgeState(id + "S", i, 0));
		edgeStates.push_back(EdgeState(id + "E", i, 1));
	}
	numVertexStates = (int)vertexStates.size();
	numEdgeStates = (int)edgeStates.size();
	numTemplateVertices = (int)templateGraph.vertices.size();

	// Build eConnections from vertices.
	for (int i = 0; i < templateGraph.numEdges; i++) {
		eConnections.push_back(vector<int>());
	}
	for (int i = 0; i < templateGraph.vertices.size(); i++) {
		auto connections = templateGraph.vertices[i].connections;
		for (int j = 0; j < connections.size(); j++) {
			int edgeIndex = connections[j];
			eConnections[edgeIndex].push_back(i);
		}
	}

	// -1 means not rejected.
	inQueue = new bool[numTemplateVertices];
	for (int i = 0; i < numTemplateVertices; i++) {
		inQueue[i] = false;
		vector <int> rejectStepOneVertex;
		const auto& templateVertex = templateGraph.vertices[i];
		const int numConnections = (int)templateVertex.connections.size();
		const bool onBoundary = !templateVertex.boundaryId.empty();
		int numStates = numStatesAtVertex(i);
		for (int j = 0; j < numStates; j++) {
			int rejectAt = -1;
			// Immediately reject any state that does not have the correct number of edges.
			if (!onBoundary && vertexStates[j].getType()->getHalfEdgeTypes().size() != numConnections) {
				rejectAt = 0;
			}
			rejectStepOneVertex.push_back(rejectAt);
		}
		rejectionStep.push_back(rejectStepOneVertex);
	}
	vIndex = 0;
}

int TemplateMatcher::numStatesAtVertex(int vIndex) {
	return templateGraph.vertices[vIndex].boundaryId.empty() ? numVertexStates : numEdgeStates;
}

const State& TemplateMatcher::getState(int vIndex, int stateIndex) const {
	return templateGraph.vertices[vIndex].boundaryId.empty()
		? static_cast<const State&>(vertexStates[stateIndex])
		: static_cast<const State&>(edgeStates[stateIndex]);
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

bool TemplateMatcher::propagate() {
	while (updateQueue.size() > 0) {
		int updateIndex = updateQueue[0];
		updateQueue.erase(updateQueue.begin());
		inQueue[updateIndex] = false;
		auto vConnections = templateGraph.vertices[updateIndex].connections;
		set<string> neighborIds;
		for (int i = 0; i < vConnections.size(); i++) {
			neighborIds.clear();
			int numStates = numStatesAtVertex(updateIndex);
			for (int j = 0; j < numStates; j++) {
				if (rejectionStep[updateIndex][j] == -1) {
					string connectionId = getState(updateIndex, j).GetConnectionId(i);
					string neighborId = HalfEdgeType::oppositeId(connectionId);
					neighborIds.insert(neighborId);
				}
			}

			int vConnection = vConnections[i];
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
					string connectionId = getState(neighbor, j).GetConnectionId(cIndex);
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

	// Add the vertices.
	auto vertexValue = vertexValues[graphIndex];
	vector<int> templateToMatch(templateGraph.vertices.size(), -1);

	for (int j = 0; j < (int)vertexValue.size(); j++) {
		templateToMatch[j] = (int)graphValues.vertices.size();
		graphValues.vertices.push_back(getState(j, vertexValue[j]).getTypeValue());
		const bool onBoundary = !templateGraph.vertices[j].boundaryId.empty();
		graphValues.vertexOnBoundary.push_back(onBoundary);
	}

	for (int j = 0; j < (int)eConnections.size(); j++) {
		auto vIndices = eConnections[j];
		// Skip boundary edges unless both vertices are boundary. It's a single edge.
		if (vIndices.size() != 2) {
			cout << "Edge should have 2 vertices." << endl;
			continue;
		}
		bool onBoundary0 = !templateGraph.vertices[vIndices[0]].boundaryId.empty();
		bool onBoundary1 = !templateGraph.vertices[vIndices[1]].boundaryId.empty();
		if (onBoundary0 != onBoundary1) {
			continue;
		}

		array<int, 4> edge{};
		size_t edgeIndex = 0;
		for (int vIndex : vIndices) {
			int cIndex = ConnectionIndex(vIndex, j, -1);
			edge[edgeIndex++] = templateToMatch[vIndex];
			edge[edgeIndex++] = getState(vIndex, vertexValue[vIndex]).GetConnectionIndex(cIndex);
		}
		graphValues.edges.push_back(edge);
	}
	return graphValues;
}
