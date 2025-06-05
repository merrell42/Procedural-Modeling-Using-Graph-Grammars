#include "pch.h"
#include "vertex.h"

namespace ms {

Vertex::Vertex(Model* model, int id, Vec3 position, VertexType* type, std::vector<int> halfedgeIds)
	: model(model)
	, id(id)
	, halfedgeIds(halfedgeIds)
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

void Vertex::createHalfEdges() {
	std::vector<Connection> connections = type->getConnections();

	for (const auto& connection : connections) {
		EdgeType* edgeType = connection.edge;
		const std::vector<FaceData> faceData = edgeType->faceData;

		for (size_t faceIndex = 0; faceIndex < faceData.size(); ++faceIndex) {
			const FaceData& faceDatum = faceData[faceIndex];
			bool position = faceDatum.onRight ^ connection.isAtStart;

			if (position) {
				createHalfEdge(connection, (int)faceIndex);
			}
		}
	}
}

HalfEdge* Vertex::createHalfEdge(const Connection& connection, int faceIndex) {
	Vec3 dir = connection.dir.copy();

	// Create the halfedge.
	const int halfedgeId = model->newId();
	const int vertexId = id;
	const int edgeId = model->newId();
	auto halfedge = new HalfEdge(model, halfedgeId, connection.isAtStart, connection.edge, dir, vertexId, -1, edgeId, true, faceIndex);

	// Create the edge.
	std::vector<int> edgeHalfEdgeIds(2, -1);
	edgeHalfEdgeIds[faceIndex] = halfedgeId;
	std::vector<int> bspNodeIds;
	auto edge = new Edge(model, edgeId, connection.edge, edgeHalfEdgeIds, bspNodeIds);

	// Add the halfedge to the vertex.
	halfedgeIds.push_back(halfedgeId);

	return halfedge;
}

Vertex* Vertex::copy() {
	auto result = new Vertex(model, id, position, type, halfedgeIds);
	return result;
}

void Vertex::destroy() {
	model->getCurrent()->removeVertex(this);
	delete this;
};

HalfEdge* Vertex::getHalfEdge(int index) const {
	return model->getCurrent()->getHalfEdge(halfedgeIds[index]);
}

std::vector<HalfEdge*> Vertex::getHalfEdges() const {
	std::vector<HalfEdge*> result;
	for (int i = 0; i < halfedgeIds.size(); i++) {
		result.push_back(getHalfEdge(i));
	}
	return result;
}

}