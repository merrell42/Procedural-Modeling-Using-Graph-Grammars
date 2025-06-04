#pragma once
#include "model.h"
#include "../geometry/vec3.h"
#include "../primitives/vertex_type.h"
#include "half_edge.h"
#include <vector>

namespace ms {

class Model;
class Endpoint;

class Vertex {
	public:
		Vertex(Model* model, int id, Vec3 position, VertexType* type, std::vector<int> endpointIds);
		Vertex(Model* model, Vec3 position, VertexType* type);
		void createEndpoints();
		std::vector<Endpoint*> getEndpoints() const;
		int getId() const { return id; }
		Endpoint* getEndpoint(int index) const;
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
		std::vector<int> endpointIds;

		Model* model;
		Endpoint* createEndpoint(const Connection& connection, int faceIndex);
};
}