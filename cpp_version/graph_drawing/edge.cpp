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

Line::Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds, std::vector<int> bspNodeIds)
	: model(model)
	, type(type)
	, id(id)
	, endpointIds(endpointIds)
	, bspNodeIds(bspNodeIds) {
	model->getCurrent()->addLine(id, this);
}

Line::~Line() {
	removeFromBsp();
}

// Define and initialize the static member
std::unordered_map<int, VertexType*> Line::splitVertexTypes;

Line* Line::copy() {
	auto result = new Line(model, id, type, endpointIds, bspNodeIds);
	return result;
}

Endpoint* Line::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Line::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

void Line::addEndpoint(Endpoint* endpoint, int index) {
	setEndpoint(index, endpoint);
	endpoint->setLine(this);
}
void Line::setEndpoint(int index, Endpoint* endpoint) {
	endpointIds[index] = endpoint->getId();
}

// New function to set endpoint IDs directly.
void Line::setEndpointIds(const std::vector<int>& ids) {
	endpointIds.resize(ids.size());
	std::copy(ids.begin(), ids.end(), endpointIds.begin());
}

void Line::destroy() {
	model->getCurrent()->removeLine(this);
	delete this;
};

SplitData Line::split(bool splitFaces) {
    timer->start("split No Vertex");

    // Create two new lines as copies of the current line.
	Line* line0 = this->copy();
	Line* line1 = this->copy();
    std::vector<Line*> newLines = { line0, line1 };
	line0->setId(model->newId());
	line1->setId(model->newId());
	model->getCurrent()->addLine(line0->getId(), line0);
	model->getCurrent()->addLine(line1->getId(), line1);
	line0->setEndpointIds({ -1, -1 });
	line1->setEndpointIds({ -1, -1 });

    // Get old endpoints and set new ones.
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    std::vector<Endpoint*> nextEndpoints;
    for (auto& endpoint : endpoints) {
        nextEndpoints.push_back(endpoint->next());
    }
	if (splitFaces) {
		for (auto& endpoint : endpoints) {
			endpoint->getFace()->split(endpoint->next());
		}
	}
    for (auto& endpoint : endpoints) {
        endpoint->transfer(newLines[endpoint->getIsAtStart() ? 0 : 1]);
    }

    destroy();
    timer->stop("split No Vertex");

    return SplitData(newLines, nextEndpoints);
}

std::pair<SplitData, Vertex*> Line::fullSplit(double s) {
	Vec3 middlePos = Vec3::lerp(
		this->getEndpoints()[0]->getPosition(),
		this->getEndpoints()[1]->getPosition(), s);

	auto edgeType = this->getEdgeType();
	auto modelCopy = model;
	SplitData split = this->split(false);
	VertexType* vertexType = Line::getVertexType(edgeType);
	Vertex* newVertex = new Vertex(modelCopy, middlePos, vertexType);
	newVertex->createEndpoints();
	
	std::vector<Line*> addedLines;
	for (auto endpoint : newVertex->getEndpoints()) {
		addedLines.push_back(endpoint->getLine());
	}
	
	std::vector<Face*> addedFaces;
	for (auto endpoint : newVertex->getEndpoints()) {
		addedFaces.push_back(endpoint->getFace());
	}

	int p = !split.lines[0]->getEndpoints()[0] ? 0 : 1;

	auto start0 = newVertex->getEndpoints()[0]->getIsAtStart();
	auto endpointT = newVertex->getEndpoints()[start0 ? 0 : 1];
	auto endpointF = newVertex->getEndpoints()[start0 ? 1 : 0];

	if (p == 0) {
		split.lines[0]->addEndpoint(endpointF, 0);
		split.lines[1]->addEndpoint(endpointT, 1);
		auto prevEndpoint0 = split.lines[0]->getEndpoints()[1];
		auto prevEndpoint1 = split.lines[1]->getEndpoints()[0];
		auto startPrev0 = prevEndpoint0->getIsAtStart();
		prevEndpoint0->getFace()->insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
		prevEndpoint1->getFace()->insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
	} else {
		split.lines[0]->addEndpoint(endpointF, 1);
		split.lines[1]->addEndpoint(endpointT, 0);
		auto prevEndpoint0 = split.lines[0]->getEndpoints()[0];
		auto prevEndpoint1 = split.lines[1]->getEndpoints()[1];
		auto startPrev0 = prevEndpoint0->getIsAtStart();
		prevEndpoint0->getFace()->insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
		prevEndpoint1->getFace()->insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
	}
	
	for (auto line : addedLines) {
		line->destroy();
	}
	for (auto face : addedFaces) {
		face->destroy();
	}

	return { split, newVertex };
}

VertexType* Line::getVertexType(EdgeType3D* edgeType) {
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

bool Line::intersects(Line* lineB) {
    auto result = Intersector::intersect(
        this->getEndpoints()[0]->getPosition().dropDim(2),
        this->getEndpoints()[1]->getPosition().dropDim(2),
        lineB->getEndpoints()[0]->getPosition().dropDim(2),
        lineB->getEndpoints()[1]->getPosition().dropDim(2)
    );
    return result.has_value();
}

Vec3* Line::getDirection() const {
	Vec3 v1(getEndpoint(1)->getPosition());
	auto normal = new Vec3(v1.minus(getEndpoint(0)->getPosition()));
	normal->normalize();
	return normal;
}

bool Line::addToBsp() {
	return model->getCurrent()->bspAddLine(this);
}

void Line::removeFromBsp() {
	for (int nodeId : bspNodeIds) {
		auto bspNode = model->getCurrent()->getBspNode(nodeId);
		if (bspNode) {
			bspNode->removeLine(this);
		}
	}
	bspNodeIds.clear();
}

}