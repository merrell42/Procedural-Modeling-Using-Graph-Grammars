#pragma once
#include <array>
#include <vector>
#include "TemplateGraph.h"
#include "json.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "State.h"

class FaceType;
using namespace std;

using Json = nlohmann::json;

enum class PrimitiveType { Vertex, Edge, Spliced };

// Mirrors json.matches[i] from ms.networkHierarchy.partialImport.
class GraphValues {
public:
	vector<int> vertices;
	vector<PrimitiveType> primitiveType;
	vector<int> spliceFaceTypeIndex;
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
	vector<EdgeType*> edgeTypes;
	vector<FaceType*> faceTypes;
	int numTemplateVertices;
	int numVertexStates;
	int numEdgeStates;
	// Our current position within the vertices.
	int vIndex;
	// For debugging.
	int counter;
	// For each graph that matches the template, this lists the value of each vertex.
	// Each value is the index of the vertex state.
	vector<vector<int>> vertexValues;

	TemplateMatcher(
		TemplateGraph templateGraph_,
		vector<VertexType*> vTypes,
		vector<EdgeType*> eTypes,
		vector<FaceType*> faceTypes_
	);
	void match();
	GraphValues getGraphValues(int graphIndex) const;

private:
	void applyDecision();
	bool propagate();
	vector<int> findChoices();
	bool findNextChoice();
	void undoLastDecision();
	void reject(int pos, int type);
	int ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex) const;
	int numStatesAtVertex(int vIndex);
	const State& getState(int vIndex, int stateIndex) const;
	void acceptMatch();
	bool propagateSplicedEdge(int updateIndex, int neighbor, int connIndex, int neighborConnIndex);
	bool propagateEdge(int updateIndex, int neighbor, int connIndex, int neighborConnIndex);

	bool usesEdgeStates(int vIndex) const;
	bool isSplicedVertex(int vIndex) const;
	bool isBoundaryVertex(int vIndex) const;
	int neighborAcrossEdge(int vIndex, int edgeIndex) const;
	string getConnectionId(int vIndex, int stateIndex, int connIndex) const;
	FaceType* faceTypeAtSpliceConnection(int vIndex, int stateIndex, int connIndex) const;
	int spliceConnectionIndex(int vIndex) const;
	bool spliceOnRight(int vIndex, int stateIndex) const;
	int splicedBoundaryVertexIndex(int vIndex, int stateIndex, int connIndex) const;
	int boundaryVertexIndex(int vIndex, int stateIndex, int templateEdgeIndex, int connIndex) const;
	int faceTypeIndex(FaceType* faceType) const;
	int splicedGraphIndex(int edgeTypeIndex, bool onRight) const;
};
