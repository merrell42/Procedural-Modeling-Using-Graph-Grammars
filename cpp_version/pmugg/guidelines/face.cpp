#include "face.h"
#include "edge.h"
#include "face_type.h"
#include "vertex.h"
#include "util.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace ms {

Face::Face(FaceType* faceType)
    : faceType(faceType)
    , edges() {
}

void Face::addEdge(Edge* edge) {
    if (!hasEdge(edge)) {
        edges.push_back(edge);
        sortEdges();
    }
}

void Face::removeEdge(Edge* edge) {
    auto it = std::find(edges.begin(), edges.end(), edge);
    if (it != edges.end()) {
        edges.erase(it);
        sortEdges();
    }
}

void Face::setFaceType(FaceType* type) {
    faceType = type;
}

FaceType* Face::getFaceType() const {
    return faceType;
}

std::vector<Edge*> Face::getEdges() const {
    return edges;
}

std::vector<Edge*> Face::getOrderedEdges() const {
    return edges; // Already maintained in sorted order
}

std::vector<Vertex*> Face::getVertices() const {
    std::vector<Vertex*> vertices;
    for (const auto& edge : edges) {
        vertices.push_back(edge->getStart()->getVertex());
    }
    return vertices;
}

Face* Face::enclosedFace() const {
    if (edges.empty()) return nullptr;
    
    auto firstEdge = edges[0];
    if (!firstEdge) return nullptr;

    auto leftFace = firstEdge->getLeftFace();
    auto rightFace = firstEdge->getRightFace();
    
    if (leftFace && leftFace != this && leftFace->isEnclosed()) {
        return leftFace;
    }
    if (rightFace && rightFace != this && rightFace->isEnclosed()) {
        return rightFace;
    }
    
    return nullptr;
}

bool Face::isEnclosed() const {
    if (edges.empty()) return false;

    // Check if all edges are connected in a loop
    for (size_t i = 0; i < edges.size(); ++i) {
        auto currentEdge = edges[i];
        auto nextEdge = edges[(i + 1) % edges.size()];
        
        if (currentEdge->getEnd()->getVertex() != nextEdge->getStart()->getVertex()) {
            return false;
        }
    }
    
    return true;
}

bool Face::isEnclosing() const {
    return enclosedFace() != nullptr;
}

void Face::print() const {
    std::cout << "Face with " << edges.size() << " edges" << std::endl;
    if (faceType) {
        std::cout << "Face type: " << faceType->getName() << std::endl;
    }
    std::cout << "Center: ";
    getCenter().print();
    std::cout << "Area: " << getArea() << std::endl;
    std::cout << "Is clockwise: " << (isClockwise() ? "true" : "false") << std::endl;
}

Edge* Face::getNextEdge(Edge* edge) const {
    int index = getEdgeIndex(edge);
    if (index == -1 || edges.empty()) {
        return nullptr;
    }
    return edges[(index + 1) % edges.size()];
}

Edge* Face::getPreviousEdge(Edge* edge) const {
    int index = getEdgeIndex(edge);
    if (index == -1 || edges.empty()) {
        return nullptr;
    }
    return edges[(index - 1 + edges.size()) % edges.size()];
}

Edge* Face::getFirstEdge() const {
    return edges.empty() ? nullptr : edges.front();
}

Edge* Face::getLastEdge() const {
    return edges.empty() ? nullptr : edges.back();
}

int Face::getEdgeCount() const {
    return static_cast<int>(edges.size());
}

bool Face::hasEdge(Edge* edge) const {
    return std::find(edges.begin(), edges.end(), edge) != edges.end();
}

Vec2 Face::getCenter() const {
    if (edges.empty()) return Vec2();
    
    Vec2 center;
    int count = 0;
    
    for (const auto& edge : edges) {
        center += edge->getStart()->getVertex()->getPosition();
        count++;
    }
    
    return center / static_cast<float>(count);
}

float Face::getArea() const {
    if (edges.size() < 3) return 0.0f;
    
    float area = 0.0f;
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& p1 = edges[i]->getStart()->getVertex()->getPosition();
        const auto& p2 = edges[(i + 1) % edges.size()]->getStart()->getVertex()->getPosition();
        area += (p1.x * p2.y - p2.x * p1.y);
    }
    
    return std::abs(area) / 2.0f;
}

bool Face::containsPoint(const Vec2& point) const {
    std::vector<Vec2> vertices;
    for (const auto& edge : edges) {
        vertices.push_back(edge->getStart()->getVertex()->getPosition());
    }
    return isPointInPolygon(point, vertices);
}

bool Face::isClockwise() const {
    if (edges.size() < 3) return false;
    
    float sum = 0.0f;
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& p1 = edges[i]->getStart()->getVertex()->getPosition();
        const auto& p2 = edges[(i + 1) % edges.size()]->getStart()->getVertex()->getPosition();
        sum += (p2.x - p1.x) * (p2.y + p1.y);
    }
    
    return sum > 0;
}

int Face::getEdgeIndex(Edge* edge) const {
    auto it = std::find(edges.begin(), edges.end(), edge);
    return it == edges.end() ? -1 : static_cast<int>(std::distance(edges.begin(), it));
}

void Face::sortEdges() {
    // Sort edges based on their connectivity
    if (edges.size() < 2) return;

    std::vector<Edge*> sortedEdges;
    sortedEdges.reserve(edges.size());
    sortedEdges.push_back(edges[0]);

    while (sortedEdges.size() < edges.size()) {
        Edge* lastEdge = sortedEdges.back();
        Vertex* lastVertex = lastEdge->getEnd()->getVertex();

        auto it = std::find_if(edges.begin(), edges.end(),
            [&](Edge* edge) {
                return edge->getStart()->getVertex() == lastVertex &&
                       std::find(sortedEdges.begin(), sortedEdges.end(), edge) == sortedEdges.end();
            });

        if (it == edges.end()) break;
        sortedEdges.push_back(*it);
    }

    edges = sortedEdges;
}

bool Face::isPointInPolygon(const Vec2& point, const std::vector<Vec2>& vertices) const {
    if (vertices.size() < 3) return false;
    
    bool inside = false;
    for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
        if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
            (point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) /
                      (vertices[j].y - vertices[i].y) + vertices[i].x)) {
            inside = !inside;
        }
    }
    
    return inside;
}

} // namespace ms 