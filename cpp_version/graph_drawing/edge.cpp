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



Edge::Edge(Model* model, int id, EdgeType* type, vector<int> halfEdgeIds, vector<int> bspNodeIds)
	: model(model)
	, type(type)
	, id(id)
	, halfEdgeIds(halfEdgeIds)
	, bspNodeIds(bspNodeIds) {
	model->getCurrent()->addEdge(id, this);
}

Edge::~Edge() {
	removeFromBsp();
}

// Define and initialize the static member
unordered_map<int, VertexType*> Edge::splitVertexTypes;

Edge* Edge::copy() {
	auto result = new Edge(model, id, type, halfEdgeIds, bspNodeIds);
	return result;
}

HalfEdge* Edge::getHalfEdge(int index) const {
	return model->getCurrent()->getHalfEdge(halfEdgeIds[index]);
}

vector<HalfEdge*> Edge::getHalfEdges() const {
	vector<HalfEdge*> result;
	for (int i = 0; i < halfEdgeIds.size(); i++) {
		result.push_back(getHalfEdge(i));
	}
	return result;
}

void Edge::addHalfEdge(HalfEdge* halfEdge, int index) {
	setHalfEdge(index, halfEdge);
	halfEdge->setEdge(this);
}
void Edge::setHalfEdge(int index, HalfEdge* halfEdge) {
	halfEdgeIds[index] = halfEdge->getId();
}

// New function to set halfEdge IDs directly.
void Edge::setHalfEdgeIds(const vector<int>& ids) {
	halfEdgeIds.resize(ids.size());
	std::copy(ids.begin(), ids.end(), halfEdgeIds.begin());
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

    // Get old halfEdges and set new ones.
    vector<HalfEdge*> halfEdges = this->getHalfEdges();
    vector<HalfEdge*> nextHalfEdges;
    for (auto& halfEdge : halfEdges) {
        nextHalfEdges.push_back(halfEdge->next());
    }
	if (splitFaces) {
		for (auto& halfEdge : halfEdges) {
			halfEdge->getFace()->split(halfEdge->next());
		}
	}
    for (auto& halfEdge : halfEdges) {
        halfEdge->transfer(newEdges[halfEdge->getIsAtStart() ? 0 : 1]);
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
	for (auto halfEdge : newVertex->getHalfEdges()) {
		addedEdges.push_back(halfEdge->getEdge());
	}
	
	vector<Face*> addedFaces;
	for (auto halfEdge : newVertex->getHalfEdges()) {
		addedFaces.push_back(halfEdge->getFace());
	}

	int p = !split.edges[0]->getHalfEdges()[0] ? 0 : 1;

	auto start0 = newVertex->getHalfEdges()[0]->getIsAtStart();
	auto halfEdgeT = newVertex->getHalfEdges()[start0 ? 0 : 1];
	auto halfEdgeF = newVertex->getHalfEdges()[start0 ? 1 : 0];

	if (p == 0) {
		split.edges[0]->addHalfEdge(halfEdgeF, 0);
		split.edges[1]->addHalfEdge(halfEdgeT, 1);
		auto prevHalfEdge0 = split.edges[0]->getHalfEdges()[1];
		auto prevHalfEdge1 = split.edges[1]->getHalfEdges()[0];
		auto startPrev0 = prevHalfEdge0->getIsAtStart();
		prevHalfEdge0->getFace()->insert(startPrev0 ? halfEdgeT : halfEdgeF, prevHalfEdge0);
		prevHalfEdge1->getFace()->insert(startPrev0 ? halfEdgeF : halfEdgeT, prevHalfEdge1);
	} else {
		split.edges[0]->addHalfEdge(halfEdgeF, 1);
		split.edges[1]->addHalfEdge(halfEdgeT, 0);
		auto prevHalfEdge0 = split.edges[0]->getHalfEdges()[0];
		auto prevHalfEdge1 = split.edges[1]->getHalfEdges()[1];
		auto startPrev0 = prevHalfEdge0->getIsAtStart();
		prevHalfEdge0->getFace()->insert(startPrev0 ? halfEdgeT : halfEdgeF, prevHalfEdge0);
		prevHalfEdge1->getFace()->insert(startPrev0 ? halfEdgeF : halfEdgeT, prevHalfEdge1);
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
        splitVertexTypes[id]->getHalfEdgeTypes()[0].edge != edgeType) {

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


int Edge::getId() const {
	return id;
}

Model* Edge::getModel() const {
	return model;
}

void Edge::setId(int newId) {
	id = newId;
}

void Edge::setBspNodeIds(const vector<int>& newBspNodeIds) {
	bspNodeIds = newBspNodeIds;
}

const vector<int>& Edge::getBspNodeIds() const {
	return bspNodeIds;
}

void Edge::addBspNodeId(int bspNodeId) {
	bspNodeIds.push_back(bspNodeId);
}