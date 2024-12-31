#pragma once
#include <vector>
#include <memory>
#include "vec2.h"

namespace ms {

class Vertex;
class Edge;
class Endpoint;
class View;
class Decoration;

class VertexGroup {
public:
    VertexGroup(float x, float y, Vertex* vertex = nullptr);
    ~VertexGroup() = default;

    // Core accessors
    const Vec2& getPosition() const { return position; }
    const std::vector<Vertex*>& getVertices() const { return vertices; }
    Decoration* getDecoration() const { return decoration; }
    bool getSelected() const { return selected; }
    bool getVisible() const { return visible; }
    bool getHovered() const { return hovered; }
    int getId() const { return id; }

    // Modifiers
    void setPosition(const Vec2& pos) { position = pos; }
    void setDecoration(Decoration* dec) { decoration = dec; }
    void setVisible(bool vis) { visible = vis; }

    // Vertex operations
    void addVertex(Vertex* vertex) { vertices.push_back(vertex); }
    void removeVertex(Vertex* vertex);
    Vertex* getVertex() const;
    void makeUnique(Vertex* vertex);

    // Group operations
    void merge(VertexGroup* groupB);
    std::vector<Vec2> getSelectedPositions() const;
    std::vector<VertexGroup*> getVertices() const;
    std::vector<VertexGroup*> getSelectedVertices() const;
    std::vector<Edge*> getEdges() const;
    std::vector<Endpoint*> getEndpoints() const;
    Endpoint* getNext(Endpoint* endpoint) const;

    // State queries
    bool isControl() const { return false; }
    bool isSnappable() const { return true; }
    bool isLarge() const { return true; }

    // Movement
    void directMove(float dx, float dy);
    void move(float dx, float dy);

    // Distance calculations
    float distance(const Vec2& p) const;
    bool isClose(const Vec2& query, float scale) const;

    // Selection
    void select();
    void deselect();
    bool hover();
    bool unhover();
    int selectType() const;

    // Drawing
    void draw(View* view, bool selected = false, bool secondPass = false);
    std::string getColor() const;

    // Static factory methods
    static VertexGroup* create(const Vec2& p);
    static VertexGroup* fromPosition(const Vec2& position);

private:
    Vec2 position;
    std::vector<Vertex*> vertices;
    Decoration* decoration;
    bool selected;
    bool visible;
    bool hovered;
    int id;

    static int nextId;
};

} // namespace ms 