#pragma once
#include <vector>
#include "TemplateGraph.h"
#include "json.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "State.h"
using namespace std;

using Json = nlohmann::json;

class Decision {
public:
	// A list of possible choices.
	vector<int> choices;
	// The index of the vertex where the choice is being made.
	int vIndex;
	Decision(int vIndex_) : vIndex(vIndex_) {}
};

class TemplateMatcher {
public:
	// The possible states at each vertex.
	vector<VertexState> vertexStates;
	// The possible states at each edge.
	vector<EdgeState> edgeStates;
	// The list of decisions.
	vector<Decision> decisions;
	// Records the step in the decision process where the state was rejected.
	// If it is at -1, the state has not been rejected.
	vector<vector<int>> rejectionStep;
	// If each vertex is in the updateQueue.
	bool* inQueue;
	vector<int> updateQueue;
	TemplateGraph templateGraph;
	// Edge connections: which vertices are connected to each edge.
	vector<vector<int>> eConnections;
	int numTemplateVertices;
	int numVertexStates;
	int numEdgeStates;
	// Our current position within the vertices.
	int vIndex;
	// For debugging.
	int counter;
	// A list of the sorted match types.
	vector<vector<int>> matchTypes;
	// A list of the match states.
	vector<vector<int>> matchStates;
	// Exclude any rules that use the same vertex types.
	bool excludeRepeats;

	TemplateMatcher(TemplateGraph templateGraph_, vector<VertexType*> vTypes, vector<EdgeType*> eTypes, bool excludeRepeats_);
	void match();
	// Add any matches to the output vector.
	void GetMatches(vector<Json>& outputVector);

private:
	void applyDecision();
	bool propagate();
	vector<int> findChoices();
	bool findNextChoice();
	void undoLastDecision();
	void reject(int pos, int type);
	int ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex);
	int numStatesAtVertex(int vIndex);
	State& getState(int vIndex, int stateIndex);
	void acceptMatch();
};
