#include "vertex_decoration.h"
#include <algorithm>

namespace ms {

VertexDecoration::VertexDecoration(const std::string& fillStyle, 
                                 const std::string& strokeStyle,
                                 const std::function<void()>& onChange)
    : Decoration(fillStyle, strokeStyle, onChange) {
    
    // Set default properties
    floatProperties["Size"] = 5.0f;
    floatProperties["StrokeWidth"] = 1.0f;
    boolProperties["Selected"] = false;
    boolProperties["Highlighted"] = false;
}

void VertexDecoration::set(const std::string& name, float value) {
    floatProperties[name] = value;
    notifyChange();
}

void VertexDecoration::set(const std::string& name, bool value) {
    boolProperties[name] = value;
    notifyChange();
}

void VertexDecoration::set(const std::string& name, const std::string& value) {
    stringProperties[name] = value;
    notifyChange();
}

float VertexDecoration::getFloat(const std::string& name) const {
    auto it = floatProperties.find(name);
    return it != floatProperties.end() ? it->second : 0.0f;
}

bool VertexDecoration::getBool(const std::string& name) const {
    auto it = boolProperties.find(name);
    return it != boolProperties.end() ? it->second : false;
}

std::string VertexDecoration::getString(const std::string& name) const {
    auto it = stringProperties.find(name);
    return it != stringProperties.end() ? it->second : "";
}

void VertexDecoration::draw(Context* context, 
                          const std::function<Vec2(const Vec2&)>& transform) {
    context->save();

    // Set up drawing styles
    context->setFillStyle(fillStyle);
    context->setStrokeStyle(strokeStyle);
    context->setLineWidth(getFloat("StrokeWidth"));

    // Draw each vertex
    for (auto* vertex : vertices) {
        drawVertex(context, vertex->getPosition(), transform);
    }

    context->restore();
}

void VertexDecoration::drawVertex(Context* context, 
                                const Vec2& position,
                                const std::function<Vec2(const Vec2&)>& transform) {
    Vec2 screenPos = transform(position);
    float size = getFloat("Size");

    context->beginPath();
    context->arc(screenPos.x, screenPos.y, size, 0, 2 * M_PI);
    context->fill();

    if (getFloat("StrokeWidth") > 0) {
        context->stroke();
    }

    // Draw selection/highlight indicators if needed
    if (getBool("Selected")) {
        context->save();
        context->setStrokeStyle("#00f");
        context->setLineWidth(2);
        context->beginPath();
        context->arc(screenPos.x, screenPos.y, size + 2, 0, 2 * M_PI);
        context->stroke();
        context->restore();
    }

    if (getBool("Highlighted")) {
        context->save();
        context->setStrokeStyle("#f00");
        context->setLineWidth(1);
        context->beginPath();
        context->arc(screenPos.x, screenPos.y, size + 4, 0, 2 * M_PI);
        context->stroke();
        context->restore();
    }
}

void VertexDecoration::addVertex(Vertex* vertex) {
    if (std::find(vertices.begin(), vertices.end(), vertex) == vertices.end()) {
        vertices.push_back(vertex);
        notifyChange();
    }
}

void VertexDecoration::removeVertex(Vertex* vertex) {
    auto it = std::find(vertices.begin(), vertices.end(), vertex);
    if (it != vertices.end()) {
        vertices.erase(it);
        notifyChange();
    }
}

const std::vector<Vertex*>& VertexDecoration::getVertices() const {
    return vertices;
}

void VertexDecoration::clearVertices() {
    if (!vertices.empty()) {
        vertices.clear();
        notifyChange();
    }
}

} // namespace ms 