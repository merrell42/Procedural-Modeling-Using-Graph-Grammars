#include "pch.h"
#include <vector>
#include <unordered_map>
#include "edge.h"
#include "vertex.h"
#include "face.h"
#include "../primitives/edge_type.h"
#include "../geometry/vec3.h"
#include "../util/timer.h"
#include "../primitives/vertex_type.h"
#include "../geometry/intersector.h"

namespace ms {

Edge::Edge(Model* model, int id, EdgeType* type, vector<int> halfedgeIds, vector<int> bspNodeIds)
	: model(model)
	, type(type)
	, id(id)
	, halfedgeIds(halfedgeIds)
	, bspNodeIds(bspNodeIds) {
	model->getCurrent()->addEdge(id, this);
}

Edge::~Edge() {
	removeFromBsp();
}

// Define and initialize the static member
unordered_map<int, VertexType*> Edge::splitVertexTypes;

Edge* Edge::copy() {
	auto result = new Edge(model, id, type, halfedgeIds, bspNodeIds);
	return result;
}

HalfEdge* Edge::getHalfEdge(int index) const {
	return model->getCurrent()->getHalfEdge(halfedgeIds[index]);
}

vector<HalfEdge*> Edge::getHalfEdges() const {
	vector<HalfEdge*> result;
	for (int i = 0; i < halfedgeIds.size(); i++) {
		result.push_back(getHalfEdge(i));
	}
	return result;
}

void Edge::addHalfEdge(HalfEdge* halfedge, int index) {
	setHalfEdge(index, halfedge);
	halfedge->setEdge(this);
}
void Edge::setHalfEdge(int index, HalfEdge* halfedge) {
	halfedgeIds[index] = halfedge->getId();
}

// New function to set halfedge IDs directly.
void Edge::setHalfEdgeIds(const vector<int>& ids) {
	halfedgeIds.resize(ids.size());
	std::copy(ids.begin(), ids.end(), halfedgeIds.begin());
}

void Edge::destroy() {
	model->getCurrent()->removeEdge(this);
	delete this;
};

SplitData Edge::split(bool splitFaces) {
    timer->start("split No Vertex");

    // Create two new edges as copies of the current edge.
	Edge* edge0 = this->copy();
	Edge* edge1 = this->copy();
    vector<Edge*> newEdges = { edge0, edge1 };
	edge0->setId(model->newId());
	edge1->setId(model->newId());
	model->getCurrent()->addEdge(edge0->getId(), edge0);
	model->getCurrent()->addEdge(edge1->getId(), edge1);
	edge0->setHalfEdgeIds({ -1, -1 });
	edge1->setHalfEdgeIds({ -1, -1 });

    // Get old halfedges and set new ones.
    vector<HalfEdge*> halfedges = this->getHalfEdges();
    vector<HalfEdge*> nextHalfEdges;
    for (auto& halfedge : halfedges) {
        nextHalfEdges.push_back(halfedge->next());
    }
	if (splitFaces) {
		for (auto& halfedge : halfedges) {
			halfedge->getFace()->split(halfedge->next());
		}
	}
    for (auto& halfedge : halfedges) {
        halfedge->transfer(newEdges[halfedge->getIsAtStart() ? 0 : 1]);
    }

    destroy();
    timer->stop("split No Vertex");

    return SplitData(newEdges, nextHalfEdges);
}

pair<SplitData, Vertex*> Edge::fullSplit(double s) {
	Vec3 middlePos = Vec3::lerp(
		this->getHalfEdges()[0]->getPosition(),
		this->getHalfEdges()[1]->getPosition(), s);

	auto edgeType = this->getEdgeType();
	auto modelCopy = model;
	SplitData split = this->split(false);
	VertexType* vertexType = Edge::getVertexType(edgeType);
	Vertex* newVertex = new Vertex(modelCopy, middlePos, vertexType);
	newVertex->createHalfEdges();
	
	vector<Edge*> addedEdges;
	for (auto halfedge : newVertex->getHalfEdges()) {
		addedEdges.push_back(halfedge->getEdge());
	}
	
	vector<Face*> addedFaces;
	for (auto halfedge : newVertex->getHalfEdges()) {
		addedFaces.push_back(halfedge->getFace());
	}

	int p = !split.edges[0]->getHalfEdges()[0] ? 0 : 1;

	auto start0 = newVertex->getHalfEdges()[0]->getIsAtStart();
	auto halfedgeT = newVertex->getHalfEdges()[start0 ? 0 : 1];
	auto halfedgeF = newVertex->getHalfEdges()[start0 ? 1 : 0];

	if (p == 0) {
		split.edges[0]->addHalfEdge(halfedgeF, 0);
		split.edges[1]->addHalfEdge(halfedgeT, 1);
		auto prevHalfEdge0 = split.edges[0]->getHalfEdges()[1];
		auto prevHalfEdge1 = split.edges[1]->getHalfEdges()[0];
		auto startPrev0 = prevHalfEdge0->getIsAtStart();
		prevHalfEdge0->getFace()->insert(startPrev0 ? halfedgeT : halfedgeF, prevHalfEdge0);
		prevHalfEdge1->getFace()->insert(startPrev0 ? halfedgeF : halfedgeT, prevHalfEdge1);
	} else {
		split.edges[0]->addHalfEdge(halfedgeF, 1);
		split.edges[1]->addHalfEdge(halfedgeT, 0);
		auto prevHalfEdge0 = split.edges[0]->getHalfEdges()[0];
		auto prevHalfEdge1 = split.edges[1]->getHalfEdges()[1];
		auto startPrev0 = prevHalfEdge0->getIsAtStart();
		prevHalfEdge0->getFace()->insert(startPrev0 ? halfedgeT : halfedgeF, prevHalfEdge0);
		prevHalfEdge1->getFace()->insert(startPrev0 ? halfedgeF : halfedgeT, prevHalfEdge1);
	}
	
	for (auto edge : addedEdges) {
		edge->destroy();
	}
	for (auto face : addedFaces) {
		face->destroy();
	}

	return { split, newVertex };
}

VertexType* Edge::getVertexType(EdgeType* edgeType) {
	const int id = edgeType->getId();
    // Check if the vertex type already exists or if it's an old version.
    if (splitVertexTypes.find(id) == splitVertexTypes.end() ||
        splitVertexTypes[id]->getConnections()[0].edge != edgeType) {

        // Spliced vertices have two edges.
		VertexType* vertexType = new VertexType();
        vertexType->addEdge(edgeType, true, edgeType->getAngle());
        vertexType->addEdge(edgeType, false, edgeType->getAngle());
        vertexType->setSpliced(true);

        splitVertexTypes[id] = vertexType;
    }
    return splitVertexTypes[id];
}

bool Edge::intersects(Edge* edgeB) {
    auto result = Intersector::intersect(
        this->getHalfEdges()[0]->getPosition().dropDim(2),
        this->getHalfEdges()[1]->getPosition().dropDim(2),
        edgeB->getHalfEdges()[0]->getPosition().dropDim(2),
        edgeB->getHalfEdges()[1]->getPosition().dropDim(2)
    );
    return result.has_value();
}

Vec3* Edge::getDirection() const {
	Vec3 v1(getHalfEdge(1)->getPosition());
	auto normal = new Vec3(v1.minus(getHalfEdge(0)->getPosition()));
	normal->normalize();
	return normal;
}

bool Edge::addToBsp() {
	return model->getCurrent()->bspAddEdge(this);
}

void Edge::removeFromBsp() {
	for (int nodeId : bspNodeIds) {
		auto bspNode = model->getCurrent()->getBspNode(nodeId);
		if (bspNode) {
			bspNode->removeEdge(this);
		}
	}
	bspNodeIds.clear();
}

}