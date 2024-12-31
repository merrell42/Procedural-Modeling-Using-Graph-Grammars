#include "edge_type.h"
#include "vertex_type.h"
#include "../decoration/area.h"
#include <iostream>
#include <algorithm>

namespace ms {

EdgeType::EdgeType(const std::string& name)
    : name(name)
    , edges()
    , connected(false)
    , brush(nullptr)
    , startVertexType(nullptr)
    , endVertexType(nullptr)
    , leftArea(nullptr)
    , rightArea(nullptr)
    , id(count++) // Initialize id using counter
{}

bool EdgeType::isConnected() const {
    return connected;
}

void EdgeType::setConnected(bool isConnected) {
    connected = isConnected;
}

void EdgeType::setBrush(Brush* newBrush) {
    brush = newBrush;
}

Brush* EdgeType::getBrush() const {
    return brush;
}

void EdgeType::setName(const std::string& newName) {
    name = newName;
}

std::string EdgeType::getName() const {
    return name;
}

void EdgeType::setStartVertexType(VertexType* vertexType) {
    startVertexType = vertexType;
}

void EdgeType::setEndVertexType(VertexType* vertexType) {
    endVertexType = vertexType;
}

VertexType* EdgeType::getStartVertexType() const {
    return startVertexType;
}

VertexType* EdgeType::getEndVertexType() const {
    return endVertexType;
}

void EdgeType::setLeftArea(Area* area) {
    leftArea = area;
}

void EdgeType::setRightArea(Area* area) {
    rightArea = area;
}

Area* EdgeType::getLeftArea() const {
    return leftArea;
}

Area* EdgeType::getRightArea() const {
    return rightArea;
}

std::vector<Edge*> EdgeType::getEdges() const {
    return edges;
}

Vec2 EdgeType::getDir() const {
    return Vec2::unitVec(angle);
}

void EdgeType::print() const {
    std::cout << "EdgeType: " << name << std::endl;
    std::cout << "Connected: " << (connected ? "true" : "false") << std::endl;
    std::cout << "Number of edges: " << edges.size() << std::endl;
    if (brush) {
        std::cout << "Has brush" << std::endl;
    }
    /* if (startVertexType) {
        std::cout << "Start vertex type: " << startVertexType->getName() << std::endl;
    }
    if (endVertexType) {
        std::cout << "End vertex type: " << endVertexType->getName() << std::endl;
    } */
    if (leftArea) {
        std::cout << "Has left area" << std::endl;
    }
    if (rightArea) {
        std::cout << "Has right area" << std::endl;
    }
}

} // namespace ms 