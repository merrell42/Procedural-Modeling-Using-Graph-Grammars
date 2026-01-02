#pragma once
#include <vector>
using namespace std;

class TemplateVertex {
public:
	// Which edges the vertex is connected to. They are ordered counter-clockwise around the vertex.
	vector<int> connections;
	bool onBoundary;
	
	// This is only used for the editor.
	Vec2 position;
};

class InputGraphTemplate {
public:
	vector<TemplateVertex> vertices;
	int numEdges;
	InputGraphTemplate(vector<TemplateVertex> vertices_, int numEdges_);
};

class GraphTemplate {
public:
	// How the vertices should be connected to edges.
	vector<vector<int>> vConnections;
	// How the edges should be connected to vertices.
	vector<vector<int>> eConnections;
	// Which edges should not be glued together.
	vector<int> brokenEdges;
	GraphTemplate(int numEdges_, vector<vector<int>> vConnections_, vector<int> brokenEdges_);
	static vector<GraphTemplate> DefaultTemplates();

	// Return which connection within vertices[vertexIndex], the edge appears.
	int ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex);
};
