#pragma once

#include <string>
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/vertex_type.h"

using namespace std;

class State {
	public:
		virtual ~State() = default;
		virtual string GetConnectionId(int connectionIndex) const = 0;
		virtual int GetConnectionIndex(int connectionIndex) const = 0;
		virtual int getRuleGeneratorId() const = 0;
		virtual string getName() const = 0;
		virtual int getTypeValue() const = 0;
};

class VertexState : public State {
	public:
		VertexState(VertexType* type_, int typeValue_, int edge0_) : type(type_), typeValue(typeValue_), edge0(edge0_) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) const override;
		int getRuleGeneratorId() const override { return type->getRuleGeneratorId(); }
		string getName() const override { return to_string(typeValue); }
		int getTypeValue() const override { return typeValue; }
		VertexType* getType() const { return type; }
		// Adjust connection index based on edge0.
		int GetConnectionIndex(int connectionIndex) const override;
	private:
		VertexType* type;
		// The orientation of the vertex. Which edge in the template is first.
		int edge0;
		int typeValue;
};

class EdgeState : public State {
	public:
		EdgeState(EdgeType* type_, string id_, int typeValue_, int halfEdgeIndex_)
			: type(type_), id(id_), oppositeId(HalfEdgeType::oppositeId(id)), typeValue(typeValue_), halfEdgeIndex(halfEdgeIndex_) {}
		// Get the edge signature for a given connection.
		string GetConnectionId(int connectionIndex) const override { return oppositeId; }
		int getRuleGeneratorId() const override { return 0; }
		string getName() const override { return id; }
		int getTypeValue() const override { return typeValue; }
		int GetConnectionIndex(int connectionIndex) const override { return halfEdgeIndex; }
		EdgeType* getType() const { return type; }
		const string& getId() const { return id; }
		const string& getOppositeId() const { return oppositeId; }
		int getHalfEdgeIndex() const { return halfEdgeIndex; }

	private:
		EdgeType* type;
		string id;
		string oppositeId;
		int typeValue;
		int halfEdgeIndex;
};

