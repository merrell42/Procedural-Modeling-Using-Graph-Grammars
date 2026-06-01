#include "pch.h"
#include "TemplateMatcher.h"
#include <set>
#include <iostream>

using namespace std;

TemplateMatcher::TemplateMatcher(
	TemplateGraph templateGraph_,
	vector<VertexType*> vTypes,
	vector<EdgeType*> eTypes,
	bool excludeRepeats_
) : templateGraph(templateGraph_) {
	excludeRepeats = excludeRepeats_;
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
		edgeStates.push_back(EdgeState(id + "S"));
		edgeStates.push_back(EdgeState(id + "E"));
	}
	numVertexStates = (int)vertexStates.size();
	numEdgeStates = (int)eTypes.size();
	numPos = (int)templateGraph.vertices.size();

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
	inQueue = new bool[numPos];
	for (int i = 0; i < numPos; i++) {
		inQueue[i] = false;
		vector <int> rejectionStepTemp;
		for (int j = 0; j < numVertexStates; j++) {
			rejectionStepTemp.push_back(-1);
		}
		rejectionStep.push_back(rejectionStepTemp);
	}
	vIndex = 0;
}

void TemplateMatcher::match() {
	Decision decison0(0);
	for (int j = 0; j < numVertexStates; j++) {
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
			for (int j = 0; j < numPos; j++) {
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

	for (int i = 0; i < numVertexStates; i++) {
		if (i != choice && rejectionStep[vIndex][i] < 0) {
			reject(vIndex, i);
		}
	}
}

int TemplateMatcher::ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex) {
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
			for (int j = 0; j < numVertexStates; j++) {
				if (rejectionStep[updateIndex][j] == -1) {
					string connectionId = vertexStates[j].GetConnectionId(i);
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
			for (int j = 0; j < numVertexStates; j++) {
				if (rejectionStep[neighbor][j] == -1) {
					string connectionId = vertexStates[j].GetConnectionId(cIndex);
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
	for (int i = 0; i < numVertexStates; i++) {
		if (rejectionStep[vIndex][i] < 0) {
			choices.push_back(i);
		}
	}
	return choices;
}

bool TemplateMatcher::findNextChoice() {
	while (true) {
		vIndex++;
		if (vIndex >= numPos) {
			vector<int> newMatchTypes;
			vector<int> newMatchStates;
			for (int i = 0; i < numPos; i++) {
				for (int j = 0; j < numVertexStates; j++) {
					if (rejectionStep[i][j] == -1) {
						newMatchTypes.push_back(vertexStates[j].getRuleGeneratorId());
						newMatchStates.push_back(j);
					}
				}
			}
			sort(newMatchTypes.begin(), newMatchTypes.end());
			bool exists = false;
			if (excludeRepeats) {
				for (int i = 0; i < matchTypes.size() && !exists; i++) {
					bool sameMatch = true;
					for (int j = 0; j < numPos; j++) {
						if (newMatchTypes[j] != matchTypes[i][j]) {
							sameMatch = false;
						}
					}
					if (sameMatch) {
						exists = true;
					}
				}
			}
			if (!exists) {
				matchTypes.push_back(newMatchTypes);
				matchStates.push_back(newMatchStates);
			}
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
	for (int i = 0; i < numPos; i++) {
		for (int j = 0; j < numVertexStates; j++) {
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

void TemplateMatcher::GetMatches(vector<Json>& outputVector) {
	for (int i = 0; i < matchStates.size(); i++) {
		Json output;

		// Add the vertices.
		auto match = matchStates[i];
		vector<int> vertexIndices;
		cout << "Match " << i << ": ";
		for (int j = 0; j < match.size(); j++) {
			cout << vertexStates[match[j]].getTypeIndex() << "(" << match[j] << ") ";
			if (!templateGraph.vertices[j].boundaryId.empty()) {
				continue;
			}
			vertexIndices.push_back(vertexStates[match[j]].getTypeIndex());
		}
		cout << endl;
		output["vertices"] = vertexIndices;

		// Add the edges.
		/* vector<Json> allEdgeIndices;
		for (int j = 0; j < eConnections.size(); j++) {
			auto vIndices = eConnections[j];
			bool skipEdge = false;
			for (int k = 0; k < vIndices.size(); k++) {
				if (!templateGraph.vertices[vIndices[k]].boundaryId.empty()) {
					skipEdge = true;
					break;
				}
			}
			if (skipEdge) {
				continue;
			}

			vector<int> edgeIndices;
			for (int k = 0; k < vIndices.size(); k++) {
				int vIndex = vIndices[k];
				int cIndex = ConnectionIndex(vIndex, j, -1);
				edgeIndices.push_back(vIndex);
				edgeIndices.push_back(states[match[vIndex]].GetConnectionIndex(cIndex));
			}
			allEdgeIndices.push_back(edgeIndices);
		}
		output["edges"] = allEdgeIndices; */

		outputVector.push_back(output);
	}
}
