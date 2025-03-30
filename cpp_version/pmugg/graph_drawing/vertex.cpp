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
	, endpointIds(type->getConnections().size(), -1)
	, position(position)
	, type(type) {
	model->getCurrent()->addVertex(id, this);
}

void Vertex::createEndpoints() {
	std::vector<Connection> connections = type->getConnections();
	int vertexIndex = 0;

	for (const auto& connection : connections) {
		EdgeType3D* edgeType = connection.edge;
		const std::vector<FaceData> faceData = edgeType->faceData;

		for (size_t faceIndex = 0; faceIndex < faceData.size(); ++faceIndex) {
			const FaceData& faceDatum = faceData[faceIndex];
			bool position = faceDatum.onRight ^ connection.isAtStart;

			if (position) {
				createEndpoint(connection, vertexIndex, faceIndex);
				++vertexIndex;
			}
		}
	}
}

Endpoint* Vertex::createEndpoint(const Connection& connection, int vertexIndex, int faceIndex) {
	/*if (node.isDestroyed()) {
		std::cerr << "Error in createEndpoint." << std::endl;
		return nullptr;
	}*/

	Vec3 dir = connection.dir.copy();
	/*if (angle != 0) {
		dir.rotate(angle);
	}*/

	// double adjustedAngle = ms::util::fixAngle(angle + connection.angle);

	// Create the endpoint
	const int endpointId = model->newId();
	const int vertexId = id;
	const int lineId = model->newId();
	// const int faceId = model->newId();
	auto endpoint = new Endpoint(model, endpointId, connection.isAtStart, connection.edge, dir, vertexId, -1, lineId, true, faceIndex);

	// Create the line and segment
	std::vector<int> lineEndpointIds(2, -1);
	lineEndpointIds[faceIndex] = endpointId;
	auto line = new Line(model, lineId, connection.edge, lineEndpointIds);

	// Add the endpoint to the vertex
	endpointIds[vertexIndex] = endpointId;

	/*endpoint->maybeMergePrevFace();
	auto twin = endpoint->twin();
	if (twin) {
		twin->maybeMergeNextFace();
	}*/

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