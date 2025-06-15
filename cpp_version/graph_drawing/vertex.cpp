#include "pch.h"
#include "vertex.h"

Vertex::Vertex(Model* model, int id, Vec3 position, VertexType* type, vector<int> halfEdgeIds)
	: model(model)
	, id(id)
	, halfEdgeIds(halfEdgeIds)
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
	vector<HalfEdgeType> halfEdgeTypes = type->getHalfEdgeTypes();

	for (const auto& halfEdgeType : halfEdgeTypes) {
		EdgeType* edgeType = halfEdgeType.edge;
		const vector<FaceData> faceData = edgeType->faceData;

		for (size_t faceIndex = 0; faceIndex < faceData.size(); ++faceIndex) {
			const FaceData& faceDatum = faceData[faceIndex];
			bool doCreate = faceDatum.onRight ^ halfEdgeType.isAtStart;
			// Most edges have two faces, we only create a half-edge half the time.
			if (doCreate) {
				createHalfEdge(halfEdgeType, (int)faceIndex);
			}
		}
	}
}

HalfEdge* Vertex::createHalfEdge(const HalfEdgeType& halfEdgetype, int faceIndex) {
	Vec3 dir = halfEdgetype.dir.copy();

	// Create the halfEdge.
	const int halfEdgeId = model->newId();
	const int vertexId = id;
	const int edgeId = model->newId();
	auto halfEdge = new HalfEdge(model, halfEdgeId, halfEdgetype.isAtStart, halfEdgetype.edge, dir, vertexId, -1, edgeId, true, faceIndex);

	// Create the edge.
	vector<int> edgeHalfEdgeIds(2, -1);
	edgeHalfEdgeIds[faceIndex] = halfEdgeId;
	vector<int> bspNodeIds;
	auto edge = new Edge(model, edgeId, halfEdgetype.edge, edgeHalfEdgeIds, bspNodeIds);

	// Add the halfEdge to the vertex.
	halfEdgeIds.push_back(halfEdgeId);

	return halfEdge;
}

Vertex* Vertex::copy() {
	auto result = new Vertex(model, id, position, type, halfEdgeIds);
	return result;
}

void Vertex::destroy() {
	model->getCurrent()->removeVertex(this);
	delete this;
};

HalfEdge* Vertex::getHalfEdge(int index) const {
	return model->getCurrent()->getHalfEdge(halfEdgeIds[index]);
}

vector<HalfEdge*> Vertex::getHalfEdges() const {
	vector<HalfEdge*> result;
	for (int i = 0; i < halfEdgeIds.size(); i++) {
		result.push_back(getHalfEdge(i));
	}
	return result;
}

int Vertex::getId() const {
	return id;
}

Vec3 Vertex::getPosition() const {
	return position;
}

void Vertex::setPosition(Vec3 newPosition) {
	position = newPosition;
}

VertexType* Vertex::getType() const {
	return type;
}
