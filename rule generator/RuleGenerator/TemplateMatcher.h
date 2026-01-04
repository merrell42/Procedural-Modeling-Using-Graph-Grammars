#pragma once
#include <vector>
#include "TemplateGraph.h"
#include "json.h"
#include "../../cpp_version/primitives/vertex_type.h"
using namespace std;

using Json = nlohmann::json;

class VertexState {
public:
	VertexType* type;
	int typeIndex;
	// The orientation of the vertex. Which edge in the template is first.
	int edge0;
	VertexState(VertexType* type_, int typeIndex_, int edge0_) : type(type_), typeIndex(typeIndex_), edge0(edge0_) { }
	// Get the edge signature for a given connection.
	string GetConnectionId(int connectionIndex);
	// Adjust connection index based on edge0.
	int GetConnectionIndex(int connectionIndex);
	// Adjust connection index based on edge0.
	int ReverseConnectionIndex(int connectionIndex);
};

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
	vector<VertexState> states;
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
	int numPos;
	int numTypes;
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

	TemplateMatcher(TemplateGraph templateGraph_, vector<VertexType*> vTypes, bool excludeRepeats_);
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
};
