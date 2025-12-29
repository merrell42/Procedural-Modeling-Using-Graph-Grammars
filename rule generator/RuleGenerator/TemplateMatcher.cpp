#include "pch.h"
#include "TemplateMatcher.h"
#include <set>
#include <iostream>

using namespace std;

int VertexState::GetConnectionIndex(int connectionIndex) {
	int n = (int)type->getHalfEdgeTypes().size();
	return (connectionIndex - edge0 + n) % n;
}

string VertexState::GetConnectionId(int connectionIndex) {
	return type->getHalfEdgeTypes()[GetConnectionIndex(connectionIndex)].getId();
}

int VertexState::ReverseConnectionIndex(int connectionIndex) {
	int n = (int)type->getHalfEdgeTypes().size();
	return (connectionIndex + edge0) % n;
}

TemplateMatcher::TemplateMatcher(GraphTemplate graphTemplate_, vector<VertexType*> vTypes, bool excludeRepeats_) : graphTemplate(graphTemplate_) {
	excludeRepeats = excludeRepeats_;
	counter = 0;
	for (int i = 0; i < vTypes.size(); i++) {
		VertexType* vType = vTypes[i];
		for (int j = 0; j < vType->getHalfEdgeTypes().size(); j++) {
			states.push_back(VertexState(vType, i, j));
		}
	}
	numTypes = (int)states.size();
	numPos = (int)graphTemplate.vConnections.size();

	// -1 means not rejected.
	inQueue = new bool[numPos];
	for (int i = 0; i < numPos; i++) {
		inQueue[i] = false;
		vector <int> rejectionStepTemp;
		for (int j = 0; j < numTypes; j++) {
			rejectionStepTemp.push_back(-1);
		}
		rejectionStep.push_back(rejectionStepTemp);
	}
	vIndex = 0;
}

void TemplateMatcher::match() {
	Decision decison0(0);
	for (int j = 0; j < numTypes; j++) {
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

	for (int i = 0; i < numTypes; i++) {
		if (i != choice && rejectionStep[vIndex][i] < 0) {
			reject(vIndex, i);
		}
	}
}

bool TemplateMatcher::propagate() {
	while (updateQueue.size() > 0) {
		int updateIndex = updateQueue[0];
		updateQueue.erase(updateQueue.begin());
		inQueue[updateIndex] = false;
		auto vConnections = graphTemplate.vConnections[updateIndex];
		set<string> neighborIds;
		for (int i = 0; i < vConnections.size(); i++) {
			neighborIds.clear();
			for (int j = 0; j < numTypes; j++) {
				if (rejectionStep[updateIndex][j] == -1) {
					string connectionId = states[j].GetConnectionId(i);
					string neighborId = HalfEdgeType::oppositeId(connectionId);
					neighborIds.insert(neighborId);
				}
			}

			int vConnection = vConnections[i];
			auto eConnections = graphTemplate.eConnections[vConnection];
			int neighbor = (eConnections[0] == updateIndex) ? eConnections[1] : eConnections[0];
			int excludeIndex = -1;
			// When the neighbor is the same as the current vertex, we have the same edge repeated
			// twice. This can happen when broken edges. Exclude the current edge. Switch to the other one.
			if (updateIndex == neighbor) {
				excludeIndex = i;
			}

			bool hasMatch = false;
			int cIndex = graphTemplate.ConnectionIndex(neighbor, vConnection, excludeIndex);
			for (int j = 0; j < numTypes; j++) {
				if (rejectionStep[neighbor][j] == -1) {
					string connectionId = states[j].GetConnectionId(cIndex);
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
	for (int i = 0; i < numTypes; i++) {
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
				for (int j = 0; j < numTypes; j++) {
					if (rejectionStep[i][j] == -1) {
						newMatchTypes.push_back(states[j].type->getRuleGeneratorId());
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
				for (int i = 0; i < numPos; i++) {
					cout << newMatchStates[i] << " ";
				}
				cout << endl;
				std::cout << "Accepted " << counter << std::endl;
				// TODO: Accept the solution.
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
		for (int j = 0; j < numTypes; j++) {
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
		for (int j = 0; j < match.size(); j++) {
			vertexIndices.push_back(states[match[j]].typeIndex);
		}
		output["vertices"] = vertexIndices;

		// Add the edges.
		vector<Json> allEdgeIndices;
		for (int j = 0; j < graphTemplate.eConnections.size(); j++) {
			auto brokenEdges = graphTemplate.brokenEdges;
			auto it = std::find(brokenEdges.begin(), brokenEdges.end(), j);
			// Skip the broken edges.
			if (it != brokenEdges.end()) {
				continue;
			}

			vector<int> edgeIndices;
			auto vIndices = graphTemplate.eConnections[j];
			for (int k = 0; k < vIndices.size(); k++) {
				int vIndex = vIndices[k];
				int cIndex = graphTemplate.ConnectionIndex(vIndex, j, -1);
				edgeIndices.push_back(vIndex);
				edgeIndices.push_back(states[match[vIndex]].GetConnectionIndex(cIndex));
			}
			allEdgeIndices.push_back(edgeIndices);
		}
		output["edges"] = allEdgeIndices;

		outputVector.push_back(output);
	}
}
