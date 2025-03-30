#pragma once
#include "model.h"
#include "../shape/vec3.h"
#include "../guidelines/vertex_type.h"
#include "endpoint.h"
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
		Endpoint* createEndpoint(const Connection& connection, int vertexIndex, int faceIndex);
};
}