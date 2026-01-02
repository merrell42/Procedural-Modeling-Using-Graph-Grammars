#include "pch.h"
#include <vector>
#include <iostream>
#include "json.h"
#include <fstream>
#include "GraphTemplate.h"
using namespace std;

using json = nlohmann::json;

GraphTemplate::GraphTemplate(
	int numEdges,
	vector<vector<int>> vConnections_,
	vector<int> brokenEdges_
) : vConnections(vConnections_), brokenEdges(brokenEdges_) {
	for (int i = 0; i < numEdges; i++) {
		eConnections.push_back(vector<int>());
	}
	for (int i = 0; i < vConnections_.size(); i++) {
		auto connections = vConnections_[i];
		for (int j = 0; j < connections.size(); j++) {
			int index = connections[j];
			eConnections[index].push_back(i);
			if (eConnections[index].size() > 2) {
				std::cout << "Edge has more than 2 vertices." << std::endl;
			}
		}
	}
}

vector<GraphTemplate> GraphTemplate::DefaultTemplates() {
	ifstream file("../graph templates/graphTemplates.txt");
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();

	vector<GraphTemplate> graphTemplates;
	json parsed = json::parse(content);
	for (int i = 0; i < parsed.size(); i++) {
		json parsedI = parsed[i];
		int numEdges = parsedI["numEdges"];
		vector<vector<int>> vertices = parsedI["vertices"];
		vector<int> brokenEdges = parsedI["brokenEdges"];
		graphTemplates.push_back(GraphTemplate(numEdges, vertices, brokenEdges));
	}
	return graphTemplates;

	/* vector<GraphTemplate> graphTemplates({
		// New wall with two turns.
		GraphTemplate(6, {{4,0,3},{5,1,0},{1,5,2},{3,2,4}}, {4,5}),

		// Two stacked walls with two turns.
		GraphTemplate(10, {{7,0,3},{8,4,1,0},{1,6,8,2},{3,2,7},{9,5,4},{5,9,6}}, {7,8,9}),

		// Two turns. Four edges each. For wall.
		GraphTemplate(14, {{10,0,6},{11,1,7,0},{12,2,8,1},{13,9,2},{6,3,10},{7,4,11,3},{8,5,12,4},{9,13,5}}, {10,11,12,13}),

		// Hipped roof, jutting out.
		GraphTemplate(17, {{15,11,0},{14,0,9,1},{1,8,2},{10,3,14,2},{13,15,3},{4,6,8},{5,16,4,9},{12,5,11},{16,7,10,6},{12,13,7}}, {14,15}),

		// Hipped roof, jutting out below the roof.
		GraphTemplate(16, {{14,11,0},{0,9,1},{1,8,2},{10,3,2},{13,14,3},{4,6,8},{5,15,4,9},{12,5,11},{15,7,10,6},{12,13,7}}, {14}),

		// Full hidden roof turn. Five edges.
		GraphTemplate(18, {{13,0,9},{14,1,10,0},{15,2,11,1},{16,3,12,2},{17,4,3},{4,17,5},{12,5,16,6},{11,6,15,7},{10,7,14,8},{9,8,13}}, {13,14,15,16,17}),

		// Box jutting out of an edge.
		GraphTemplate(12, {{11,3,0},{0,1,9},{2,8,1},{10,2,3},{8,7,4},{9,4,5},{6,11,5},{10,6,7}}, {11}),
		// Four-sided object with flat top.
		GraphTemplate(12, {{7,0,6},{1,4,0},{8,2,1},{2,9,3},{4,3,5},{6,5,10},{11,8,7},{11,10,9}}, {}),

		// Eight-sided object with flat top.
		GraphTemplate(24, {{7,0,8},{0,1,23},{2,22,1},{3,21,2},{4,20,3},{19,4,5},{18,5,6},{6,7,17},{16,8,9},{9,23,10},{22,11,10},{12,11,21},{13,12,20},{13,19,14},{14,18,15},{15,17,16}}, {}),

		// Split a face with two edges into four faces.
		GraphTemplate(12, {{10,0,9},{11,1,0},{1,8,2},{9,2,3},{3,4,7},{8,5,4},{5,11,6},{7,6,10}}, {10,11}),

		// Four-sided object with pyramid top.
		// GraphTemplate(16, {{3,0,2,10},{4,5,1,0},{2,1,6,12},{8,7,4,3},{7,13,6,5},{15,8,9},{9,10,11},{11,12,14},{15,14,13}}, {}),

		// Four-sided. Hipped roof, linear top.
		GraphTemplate(17, {{0,8,3},{1,9,0},{1,2,10},{3,11,2},{4,12,7,8},{9,5,13,4},{5,10,6,14},{7,15,6,11},{13,16,12},{16,14,15}}, {}),

		// Four-sided. Wall. Four edges on each side.
		GraphTemplate(28, {{7,0,25},{27,1,24,0},{26,2,23,1},{3,8,2},{4,10,3},{5,18,4,26},{6,21,5,27},{22,6,7},{8,9,11},{10,17,9},{23,11,19,12},{24,12,20,13},{25,13,14},{22,14,15},{21,15,20,16},{18,16,19,17}}, {}),

		// GraphTemplate(17, {{1,12,0,6},{0,14,2,8},{4,3,13,1},{3,9,2,15},{11,4,5},{5,6,7},{7,8,10},{11,10,9},{13,16,12},{16,15,14}}, {}),
		// Reversed.
		// GraphTemplate(17, {{14,7,0,2},{0,3,1},{2,1,8,13},{4,6,3},{15,5,4,7},{6,5,16,8},{9,13,12},{10,14,9},{11,15,10},{16,11,12}}, {}),
	});
	return graphTemplates; */
}

// vertexIndex = neighbor
int GraphTemplate::ConnectionIndex(int vertexIndex, int edgeIndex, int excludeIndex) {
	auto nConnections = vConnections[vertexIndex];

	for (int i = 0; i < nConnections.size(); i++) {
		if (nConnections[i] == edgeIndex && i != excludeIndex) {
			return i;
		}
	}
	cout << "Connection not found." << endl;
	return -1;
}