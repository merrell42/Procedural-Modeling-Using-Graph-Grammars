#pragma once

#include <string>
#include "../../cpp_version/primitives/vertex_type.h"

using namespace std;

class State {
	public:
		virtual ~State() = default;
		virtual string GetConnectionId(int connectionIndex) = 0;
		virtual int getRuleGeneratorId() = 0;
		virtual string getName() = 0;
};

class VertexState : public State {
	public:
		VertexState(VertexType* type_, int typeIndex_, int edge0_) : type(type_), typeIndex(typeIndex_), edge0(edge0_) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) override;
		int getRuleGeneratorId() override { return type->getRuleGeneratorId(); }
		string getName() override { return to_string(typeIndex); }
	private:
		// Adjust connection index based on edge0.
		int GetConnectionIndex(int connectionIndex);
		VertexType* type;
		// The orientation of the vertex. Which edge in the template is first.
		int edge0;
		int typeIndex;
};

class EdgeState : public State {
	public:
		EdgeState(string id_) : id(id_), oppositeId(HalfEdgeType::oppositeId(id)) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) override { return oppositeId; }
		int getRuleGeneratorId() override { return 0; }
		string getName() override { return id; }

	private:
		string id;
		string oppositeId;
};

