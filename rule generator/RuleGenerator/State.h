#pragma once

#include <string>
#include "../../cpp_version/primitives/vertex_type.h"

using namespace std;

class State {
	public:
		virtual ~State() = default;
		virtual string GetConnectionId(int connectionIndex) const = 0;
		virtual int getRuleGeneratorId() const = 0;
		virtual string getName() const = 0;
		virtual int getTypeIndex() const = 0;
};

class VertexState : public State {
	public:
		VertexState(VertexType* type_, int typeIndex_, int edge0_) : type(type_), typeIndex(typeIndex_), edge0(edge0_) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) const override;
		int getRuleGeneratorId() const override { return type->getRuleGeneratorId(); }
		string getName() const override { return to_string(typeIndex); }
		int getTypeIndex() const override { return typeIndex; }
		// Adjust connection index based on edge0.
		int GetConnectionIndex(int connectionIndex) const;
	private:
		VertexType* type;
		// The orientation of the vertex. Which edge in the template is first.
		int edge0;
		int typeIndex;
};

class EdgeState : public State {
	public:
		EdgeState(string id_) : id(id_), oppositeId(HalfEdgeType::oppositeId(id)) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) const override { return oppositeId; }
		int getRuleGeneratorId() const override { return 0; }
		string getName() const override { return id; }
		int getTypeIndex() const override { return -1; }

	private:
		string id;
		string oppositeId;
};

