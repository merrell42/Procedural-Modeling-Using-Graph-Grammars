#pragma once
#include <array>
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
	vector<int> vertices;
	vector<bool> vertexOnBoundary;
	// Spliced template vertices: use a 3-spoke spliced primitive (not edge/vertex).
	vector<bool> vertexSpliced;
	// For spliced sites: which FaceData side of the segment the splice uses.
	vector<bool> spliceOnRight;
	// For spliced sites: whether this site is the start end of the spliced edge.
	vector<bool> spliceIsAtStart;
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
	// Boundary or spliced vertices are assigned edge states.
	bool usesEdgeState(int vIndex) const;
	// Face-id advertised on a spliced half-edge for the given edge state (empty if none).
	string faceIdForSplicedConnection(int vIndex, int stateIndex, int connIndex) const;
	// Which FaceData side the spliced half-edge selects for this edge state.
	// Returns false if the connection/state cannot select a face.
	bool splicedFaceOnRight(int vIndex, int stateIndex, int connIndex, bool& onRightOut) const;
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
	// Oriented walk along the two normal half-edges of a spliced vertex for this edge state.
	// fromConn = start side of the oriented edge; toConn = end side.
	bool orientedNormalWalk(int vIndex, int stateIndex, int& fromConn, int& toConn) const;
	// Resolves left/right for a spliced half-edge. Returns false if not applicable.
	bool selectSplicedFaceSide(int vIndex, int stateIndex, int connIndex, bool& onRightOut) const;
	// Half-edge connection id at a template vertex/state/connection (spliced-aware).
	string connectionIdAt(int vIndex, int stateIndex, int connIndex) const;
	// bVertex / half index at a connection (spliced-aware).
	int connectionIndexAt(int vIndex, int stateIndex, int connIndex) const;
};
