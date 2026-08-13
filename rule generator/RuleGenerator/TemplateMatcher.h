#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>
#include "TemplateGraph.h"
#include "json.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "State.h"
using namespace std;

using Json = nlohmann::json;

class EdgeType;
class FaceType;

// Match values describing one template assignment, used to glue primitive
// graphs into a rule side (see buildGraphFromValues). Evolved from the web
// matcher output that ms.networkHierarchy.glueMatch consumed as json.matches[i].
class GraphValues {
public:
	struct Site {
		int typeValue = 0;
		enum Kind { Interior, Boundary, Spliced } kind = Interior;
		bool spliceOnRight = false;
		bool spliceIsAtStart = true;
	};
	vector<Site> vertices;
	vector<array<int, 4>> edges;
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
	// Edge types used to resolve FaceData at spliced vertices.
	vector<EdgeType*> edgeTypes;
	int numTemplateVertices;
	int numVertexStates;
	int numEdgeStates;
	// Our current position within the vertices.
	int vIndex;
	// For each graph that matches the template, this lists the value of each vertex.
	// Each value is the index of the vertex state.
	vector<vector<int>> vertexValues;

	TemplateMatcher(TemplateGraph templateGraph_, vector<VertexType*> vTypes, vector<EdgeType*> eTypes);
	void match();
	GraphValues getGraphValues(int graphIndex) const;

private:
	struct SpliceLayout {
		int fromConn = -1;
		int toConn = -1;
		int spliceConn = -1;
		int spliceEdge = -1;
		bool onRight = false;
		string startId;
		string endId;
		string faceId;
		int startB = 0;
		int endB = 1;
		static constexpr int spliceB = 2;
	};

	// Boundary or spliced vertices are assigned edge states.
	bool usesEdgeState(int vIndex) const;
	optional<SpliceLayout> spliceLayout(int vIndex, int stateIndex) const;
	// Half-edge id (or face id on a spliced spoke) advertised at a connection.
	string advertisedId(int vIndex, int stateIndex, int connIndex) const;
	// bVertex / half index at a connection (spliced-aware).
	int advertisedBIndex(int vIndex, int stateIndex, int connIndex) const;
	void applyDecision();
	bool propagate();
	vector<int> findChoices();
	bool findNextChoice();
	void undoLastDecision();
	void reject(int pos, int type);
	int ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex) const;
	int numStatesAtVertex(int vIndex) const;
	const State& getState(int vIndex, int stateIndex) const;
	void acceptMatch();
	static string faceTypeId(FaceType* type);
};
