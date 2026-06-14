#include "pch.h"
#include "graph_face.h"
#include "graph_half_edge.h"
#include "graph.h"
#include "../geometry/vec3.h"
#include "../util/binary_stream.h"
#include "../util/util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

GraphFace::GraphFace() 
    : outerComponent(nullptr)
    , type(nullptr)
    , graph(nullptr) {
}

GraphHalfEdge* GraphFace::getOuterComponent() const {
    return outerComponent;
}

Graph* GraphFace::getGraph() const {
    return graph;
}

GraphFace* GraphFace::connectGraph(Graph* graph) {
    this->graph = graph;
    graph->addFace(this);
    return this;
}

void GraphFace::import(const Json & json) {
    auto halfEdges = graph->getHalfEdges();
    if (halfEdges.size() == 0) {
        return;
    }
    outerComponent = graph->getHalfEdges()[json["outerComponent"]];

    // innerComponents.clear();
    // for (const auto& halfEdge : json["innerComponents"]) {
    //    Vec3 dir = Vec3::import(halfEdge);
    //    // Create a lambda to mimic the getDir behavior
    //    innerComponents.push_back(new GraphHalfEdge(dir));
    // }
}

Json GraphFace::exportJson(const vector<GraphHalfEdge*>& graphHalfEdges) const {
    Json json;
    json["outerComponent"] = outerComponent ?
        indexOf(graphHalfEdges, outerComponent) : -1;
    return json;
}

// Get the connected half-edges on a face.
// Continue until the path ends or the path loops.
vector<GraphHalfEdge*> GraphFace::getConnectedHalfEdges(const GraphHalfEdge* start) {
    vector<GraphHalfEdge*> result;
    const GraphHalfEdge* current = start;
    do {
        result.push_back(const_cast<GraphHalfEdge*>(current));
        current = current->getNext();
    } while (current && current != start);
    return result;
}

vector<GraphHalfEdge*> GraphFace::getOuterHalfEdges() const {
    if (outerComponent) {
        return getConnectedHalfEdges(outerComponent);
    }
    return vector<GraphHalfEdge*>();
}

int GraphFace::computeTurns() const {
    GraphHalfEdge* halfEdge = outerComponent;
    if (!halfEdge) {
        return 0;
    }
    vector<GraphHalfEdge*> faceHalfs = getConnectedHalfEdges(halfEdge);
    bool looped = faceHalfs.back()->getNext() == faceHalfs.front();
    if (!looped) {
        faceHalfs.pop_back();
    }
    vector<double> angles;
    angles.reserve(faceHalfs.size());
    for (auto* half : faceHalfs) {
        angles.push_back(half->getAngle());
    }
    reverse(angles.begin(), angles.end());
    auto oppositeAngle = [](double angle) {
        return Util::fixAngle(angle + M_PI);
    };
    for (auto& angle : angles) {
        angle = oppositeAngle(angle);
    }
    if (!looped) {
        angles.insert(angles.begin(), oppositeAngle(angles[0]));
    } else {
        angles.push_back(angles[0]);
    }
    return Util::wedgeTurns(angles);
}

// Replace half-edge a with b.
void GraphFace::replaceHalfEdge(GraphHalfEdge* a, GraphHalfEdge* b) {
    if (outerComponent == a) {
        outerComponent = b;
    }
    
    /* for (size_t i = 0; i < innerComponents.size(); i++) {
        if (innerComponents[i] == a) {
            innerComponents[i] = b;
        }
    } */
    b->setFace(this);
}

void GraphFace::mergeInto(GraphFace* other) {
    if (!other || other == this) {
        return;
    }
    auto halfEdges = getConnectedHalfEdges(other->getOuterComponent());
    for (auto* half : halfEdges) {
        half->setFace(this);
    }
}

void GraphFace::setOuterComponent(GraphHalfEdge* half) {
    outerComponent = half;
    if (half) {
        half->setFace(this);
    }
}

bool GraphFace::isLoopy() const {
    return outerComponent->isLoopy();
}

void GraphFace::binaryDeserialize(std::istream& in) {
    int32_t idx = bsRead<int32_t>(in);
    outerComponent = idx >= 0 ? graph->getHalfEdges()[idx] : nullptr;
}

void GraphFace::setType(FaceType* newType) {
    type = newType;
}
