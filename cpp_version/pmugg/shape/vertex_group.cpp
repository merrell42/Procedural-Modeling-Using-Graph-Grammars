#include "vertex_group.h"
#include "vertex.h"
#include "edge.h"
#include "endpoint.h"
#include "decoration.h"
#include "view.h"
#include "shape.h"
#include "util.h"
#include <algorithm>

namespace ms {

int VertexGroup::nextId = 0;

VertexGroup::VertexGroup(float x, float y, Vertex* vertex)
    : position(x, y)
    , decoration(nullptr)
    , selected(false)
    , visible(true)
    , hovered(false)
    , id(nextId++) {
    if (vertex) {
        vertices.push_back(vertex);
    }
}

void VertexGroup::removeVertex(Vertex* vertex) {
    auto it = std::find(vertices.begin(), vertices.end(), vertex);
    if (it != vertices.end()) {
        vertices.erase(it);
    }
}

Vertex* VertexGroup::getVertex() const {
    if (vertices.size() != 1) {
        throw std::runtime_error("Wrong number of vertices in getVertex");
    }
    return vertices[0];
}

void VertexGroup::makeUnique(Vertex* vertex) {
    if (vertices.size() > 1) {
        removeVertex(vertex);
        vertex->setGroup(new VertexGroup(position.getX(), position.getY(), vertex));
    }
}

void VertexGroup::merge(VertexGroup* groupB) {
    vertices.insert(vertices.end(), groupB->vertices.begin(), groupB->vertices.end());
    
    for (auto* vertex : groupB->vertices) {
        vertex->setGroup(this);
    }
    
    groupB->vertices.clear();
    visible = true;
    selected = false;

    auto* decorationB = groupB->getDecoration();
    if (decorationB) {
        auto imageB = decorationB->get("Image");
        if (imageB && imageB != "None") {
            decoration = decorationB;
        }
    }
}

std::vector<Vec2> VertexGroup::getSelectedPositions() const {
    return {position};
}

std::vector<VertexGroup*> VertexGroup::getVertices() const {
    return {const_cast<VertexGroup*>(this)};
}

std::vector<VertexGroup*> VertexGroup::getSelectedVertices() const {
    return {const_cast<VertexGroup*>(this)};
}

std::vector<Edge*> VertexGroup::getEdges() const {
    std::vector<Edge*> edges;
    for (auto* vertex : vertices) {
        auto vertexEdges = vertex->getEdges();
        edges.insert(edges.end(), vertexEdges.begin(), vertexEdges.end());
    }
    return edges;
}

std::vector<Endpoint*> VertexGroup::getEndpoints() const {
    std::vector<Endpoint*> endpoints;
    for (auto* vertex : vertices) {
        auto vertexEndpoints = vertex->getEndpoints();
        endpoints.insert(endpoints.end(), vertexEndpoints.begin(), vertexEndpoints.end());
    }
    return endpoints;
}

Endpoint* VertexGroup::getNext(Endpoint* endpoint) const {
    auto endpoints = getEndpoints();
    std::sort(endpoints.begin(), endpoints.end(), 
        [](Endpoint* a, Endpoint* b) { return a->tangent() < b->tangent(); });
    
    auto it = std::find(endpoints.begin(), endpoints.end(), endpoint);
    if (it == endpoints.end()) {
        std::cerr << "Endpoint not found" << std::endl;
        return nullptr;
    }
    
    return endpoints[(it - endpoints.begin() + 1) % endpoints.size()];
}

void VertexGroup::directMove(float dx, float dy) {
    position.move(dx, dy);
}

void VertexGroup::move(float dx, float dy) {
    if (selected) {
        position.move(dx, dy);
        for (auto* vertex : vertices) {
            vertex->move(dx, dy);
        }
    }
}

float VertexGroup::distance(const Vec2& p) const {
    return position.distance(p);
}

bool VertexGroup::isClose(const Vec2& query, float scale) const {
    return query.distance2(position) < Shape::NEAR_RADIUS2 / (scale * scale);
}

void VertexGroup::select() {
    selected = true;
    for (auto* vertex : vertices) {
        vertex->select();
    }
}

void VertexGroup::deselect() {
    selected = false;
}

bool VertexGroup::hover() {
    bool changed = !hovered;
    hovered = true;
    return changed;
}

bool VertexGroup::unhover() {
    bool changed = hovered;
    hovered = false;
    return changed;
}

int VertexGroup::selectType() const {
    return Shape::SelectableTypes::VERTEX;
}

void VertexGroup::draw(View* view, bool selected, bool secondPass) {
    view->drawVertex(this, selected, secondPass);
}

std::string VertexGroup::getColor() const {
    if (selected) {
        return Shape::SELECTED_COLOR;
    }
    return hovered ? Shape::HOVERED_COLOR : Shape::DEFAULT_COLOR;
}

VertexGroup* VertexGroup::create(const Vec2& p) {
    auto* result = new VertexGroup(0, 0);
    result->position = p;
    return result;
}

VertexGroup* VertexGroup::fromPosition(const Vec2& position) {
    auto* result = new VertexGroup(0, 0);
    result->position = position;
    return result;
}

} // namespace ms 