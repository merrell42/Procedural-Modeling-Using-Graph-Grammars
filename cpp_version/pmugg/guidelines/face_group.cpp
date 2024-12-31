#include "face_group.h"
#include "face.h"
#include "face_type.h"
#include "edge.h"
#include "vertex.h"
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace ms {

FaceGroup::FaceGroup(const std::string& name)
    : name(name)
    , faces() {
}

void FaceGroup::addFace(Face* face) {
    if (face && !hasFace(face)) {
        faces.push_back(face);
    }
}

void FaceGroup::removeFace(Face* face) {
    auto it = std::find(faces.begin(), faces.end(), face);
    if (it != faces.end()) {
        faces.erase(it);
    }
}

void FaceGroup::setName(const std::string& newName) {
    name = newName;
}

std::string FaceGroup::getName() const {
    return name;
}

std::vector<Face*> FaceGroup::getFaces() const {
    return faces;
}

std::vector<Edge*> FaceGroup::getEdges() const {
    return collectEdges();
}

std::vector<Vertex*> FaceGroup::getVertices() const {
    return collectVertices();
}

bool FaceGroup::hasFace(Face* face) const {
    return std::find(faces.begin(), faces.end(), face) != faces.end();
}

void FaceGroup::addFaces(const std::vector<Face*>& newFaces) {
    for (auto* face : newFaces) {
        addFace(face);
    }
}

void FaceGroup::removeFaces(const std::vector<Face*>& facesToRemove) {
    for (auto* face : facesToRemove) {
        removeFace(face);
    }
}

void FaceGroup::clear() {
    faces.clear();
}

bool FaceGroup::isEmpty() const {
    return faces.empty();
}

size_t FaceGroup::size() const {
    return faces.size();
}

Vec2 FaceGroup::getCenter() const {
    if (faces.empty()) return Vec2();
    
    Vec2 center;
    int count = 0;
    
    for (const auto* face : faces) {
        center += face->getCenter();
        count++;
    }
    
    return center / static_cast<float>(count);
}

float FaceGroup::getArea() const {
    float totalArea = 0.0f;
    for (const auto* face : faces) {
        totalArea += face->getArea();
    }
    return totalArea;
}

bool FaceGroup::containsPoint(const Vec2& point) const {
    for (const auto* face : faces) {
        if (face->containsPoint(point)) {
            return true;
        }
    }
    return false;
}

void FaceGroup::print() const {
    std::cout << "FaceGroup: " << name << std::endl;
    std::cout << "Number of faces: " << faces.size() << std::endl;
    std::cout << "Center: ";
    getCenter().print();
    std::cout << "Total area: " << getArea() << std::endl;
    
    std::cout << "Edges: " << getEdges().size() << std::endl;
    std::cout << "Vertices: " << getVertices().size() << std::endl;
}

std::vector<Edge*> FaceGroup::collectEdges() const {
    std::unordered_set<Edge*> uniqueEdges;
    for (const auto* face : faces) {
        auto faceEdges = face->getEdges();
        uniqueEdges.insert(faceEdges.begin(), faceEdges.end());
    }
    return std::vector<Edge*>(uniqueEdges.begin(), uniqueEdges.end());
}

std::vector<Vertex*> FaceGroup::collectVertices() const {
    std::unordered_set<Vertex*> uniqueVertices;
    for (const auto* face : faces) {
        auto faceVertices = face->getVertices();
        uniqueVertices.insert(faceVertices.begin(), faceVertices.end());
    }
    return std::vector<Vertex*>(uniqueVertices.begin(), uniqueVertices.end());
}

} // namespace ms 