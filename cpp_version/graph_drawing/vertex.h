#pragma once
#include "model.h"
#include "../memory_counter.h"
#include "../geometry/vec3.h"
#include "../primitives/vertex_type.h"
#include "half_edge.h"
#include <vector>

class Model;
class HalfEdge;

class Vertex {
	public:
		Vertex(Model* model, int id, Vec3 position, VertexType* type, vector<int> halfEdgeIds);
		Vertex(Model* model, Vec3 position, VertexType* type);
		~Vertex();
		vector<HalfEdge*> getHalfEdges() const;
		int getId() const;
		HalfEdge* getHalfEdge(int index) const;
		Vec3 getPosition() const;
		Vertex* copy();
		VertexType* getType() const;

		void createHalfEdges();
		void setPosition(Vec3 newPosition);
		void destroy();

	private:
		Model* model;
		int id;
		Vec3 position;
		VertexType* type;
		vector<int> halfEdgeIds;

		HalfEdge* createHalfEdge(const HalfEdgeType& halfEdgetype, int faceIndex);
};
