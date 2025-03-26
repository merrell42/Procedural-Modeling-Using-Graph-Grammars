#include "line.h"
#include "vertex.h"
#include "face.h"
#include "../shapes3D/edge_type3d.h"
#include "../shape/vec3.h"
#include <vector>
#include "../util/timer.h"
#include "../guidelines/vertex_type.h"
#include "../shapes3D/edge_type3d.h"
#include <unordered_map>

namespace ms {

Line::Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds)
	: model(model)
	, type(type)
	, id(id)
	, endpointIds(endpointIds) {
	model->getCurrent()->addLine(id, this);
}

// Define and initialize the static member
std::unordered_map<int, VertexType*> Line::splitVertexTypes;

Line* Line::copy() {
	auto result = new Line(model, id, type, endpointIds);
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

// New function to set endpoint IDs directly
void Line::setEndpointIds(const std::vector<int>& ids) {
	// Resize endpointIds to match the size of ids if necessary
	endpointIds.resize(ids.size());
	// Set the endpointIds directly
	std::copy(ids.begin(), ids.end(), endpointIds.begin());
}

void Line::destroy() {
	model->getCurrent()->removeLine(this);
	delete this;
};

SplitData Line::split() {
    timer->start("split No Vertex"); // Start the timer
    // auto stats = this->node->getStats(); // Get stats from the node

    // Create two new lines as copies of the current line
	Line* line0 = this->copy();
	Line* line1 = this->copy();
    std::vector<Line*> newLines = { line0, line1 };
	line0->setId(model->newId());
	line1->setId(model->newId());
	model->getCurrent()->addLine(line0->getId(), line0);
	model->getCurrent()->addLine(line1->getId(), line1);
	line0->setEndpointIds({ -1, -1 });
	line1->setEndpointIds({ -1, -1 });

	
    /* newLines[0]->addSegments({ new LineSegment(stats) });
    newLines[1]->addSegments({ new LineSegment(stats) }); */

    // Get old endpoints and set new ones
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    std::vector<Endpoint*> nextEndpoints;
    for (auto& endpoint : endpoints) {
        nextEndpoints.push_back(endpoint->next());
    }

    for (auto& endpoint : endpoints) {
        endpoint->getFace()->split(endpoint->next());
    }

    for (auto& endpoint : endpoints) {
        endpoint->transfer(newLines[endpoint->getIsAtStart() ? 0 : 1]);
    }

    destroy();
    timer->stop("split No Vertex"); // Stop the timer

    return SplitData(newLines, nextEndpoints); // Return the SplitData struct
}

std::pair<SplitData, Vertex*> Line::fullSplit(double s) {
	Vec3 middlePos = Vec3::lerp(
		this->getEndpoints()[0]->getPosition(),
		this->getEndpoints()[1]->getPosition(), s);

	auto edgeType = this->getEdgeType();
	auto modelCopy = model;
	SplitData split = this->split();
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
		prevEndpoint0->maybeMergeNextFace();
		prevEndpoint1->maybeMergeNextFace();
	} else {
		split.lines[0]->addEndpoint(endpointF, 1);
		split.lines[1]->addEndpoint(endpointT, 0);
		auto prevEndpoint0 = split.lines[0]->getEndpoints()[0];
		auto prevEndpoint1 = split.lines[1]->getEndpoints()[1];
		auto startPrev0 = prevEndpoint0->getIsAtStart();
		prevEndpoint0->getFace()->insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
		prevEndpoint1->getFace()->insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
		prevEndpoint0->maybeMergeNextFace();
		prevEndpoint1->maybeMergeNextFace();
	}

	/* split.lines[0]->fillFromEndpoints(true);
	split.lines[1]->fillFromEndpoints(true); */
	
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
    // Check if the vertex type already exists or if it's an old version
    if (splitVertexTypes.find(id) == splitVertexTypes.end() ||
        splitVertexTypes[id]->getConnections()[0].edge != edgeType) {
        
        VertexType* vertexType = new VertexType();
        std::vector<int> faceIds; // Assuming faceIds is a vector of integers

        // Add edges to the vertex type
        vertexType->addEdge(edgeType, true, edgeType->getAngle(), faceIds);
        vertexType->addEdge(edgeType, false, edgeType->getAngle(), faceIds);
        vertexType->setSpliced(true);

        // Store the new vertex type
        splitVertexTypes[id] = vertexType;
    }

    return splitVertexTypes[id]; // Return the vertex type
}

}