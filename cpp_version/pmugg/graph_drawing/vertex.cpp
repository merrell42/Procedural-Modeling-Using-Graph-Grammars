#include "pch.h"
#include "vertex.h"

namespace ms {

Vertex::Vertex(Model* model, int id, Vec3 position, VertexType* type, std::vector<int> endpointIds)
	: model(model)
	, id(id)
	, endpointIds(endpointIds)
	, position(position)
	, type(type) {
	model->getCurrent()->addVertex(id, this);
}

Vertex::Vertex(Model* model, Vec3 position, VertexType* type)
	: model(model)
	, id(model->newId())
	, position(position)
	, type(type) {
	model->getCurrent()->addVertex(id, this);
}

void Vertex::createEndpoints() {
	std::vector<Connection> connections = type->getConnections();

	for (const auto& connection : connections) {
		EdgeType3D* edgeType = connection.edge;
		const std::vector<FaceData> faceData = edgeType->faceData;

		for (size_t faceIndex = 0; faceIndex < faceData.size(); ++faceIndex) {
			const FaceData& faceDatum = faceData[faceIndex];
			bool position = faceDatum.onRight ^ connection.isAtStart;

			if (position) {
				createEndpoint(connection, faceIndex);
			}
		}
	}
}

Endpoint* Vertex::createEndpoint(const Connection& connection, int faceIndex) {
	Vec3 dir = connection.dir.copy();

	// Create the endpoint.
	const int endpointId = model->newId();
	const int vertexId = id;
	const int lineId = model->newId();
	auto endpoint = new Endpoint(model, endpointId, connection.isAtStart, connection.edge, dir, vertexId, -1, lineId, true, faceIndex);

	// Create the line.
	std::vector<int> lineEndpointIds(2, -1);
	lineEndpointIds[faceIndex] = endpointId;
	std::vector<int> bspNodeIds;
	auto line = new Line(model, lineId, connection.edge, lineEndpointIds, bspNodeIds);

	// Add the endpoint to the vertex.
	endpointIds.push_back(endpointId);

	return endpoint;
}

Vertex* Vertex::copy() {
	auto result = new Vertex(model, id, position, type, endpointIds);
	return result;
}

void Vertex::destroy() {
	model->getCurrent()->removeVertex(this);
	delete this;
};

Endpoint* Vertex::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Vertex::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

}