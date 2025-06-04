#pragma once
#include "model.h"
#include "../geometry/vec3.h"
#include "../primitives/vertex_type.h"
#include "half_edge.h"
#include <vector>

namespace ms {

class Model;
class HalfEdge;

class Vertex {
	public:
		Vertex(Model* model, int id, Vec3 position, VertexType* type, std::vector<int> halfedgeIds);
		Vertex(Model* model, Vec3 position, VertexType* type);
		void createHalfEdges();
		std::vector<HalfEdge*> getHalfEdges() const;
		int getId() const { return id; }
		HalfEdge* getHalfEdge(int index) const;
		Vec3 getPosition() const { return position; }
		void setPosition(Vec3 newPosition) { position = newPosition; }
		Vertex* copy();
		VertexType* getType() const {
			return type;
		}
		void destroy();

	private:
		int id;
		Vec3 position;
		VertexType* type;
		std::vector<int> halfedgeIds;

		Model* model;
		HalfEdge* createHalfEdge(const Connection& connection, int faceIndex);
};
}