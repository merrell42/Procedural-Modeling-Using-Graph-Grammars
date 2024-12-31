#pragma once
#include <memory>
#include "vec2.h"

namespace ms {

class Vertex;
class Endpoint;
class Shape;
class View;
class Brush;
struct DrawOptions;

class Edge {
public:
    Edge(Vertex* startVertex, Vertex* endVertex);
    ~Edge() = default;

    // Core accessors
    Endpoint* getStart() const { return start.get(); }
    Endpoint* getEnd() const { return end.get(); }
    Shape* getShape() const { return shape; }
    bool getSelected() const { return selected; }
    Brush* getBrush() const { return brush; }
    int getId() const { return id; }
    int getKey() const { return key; }

    // Modifiers
    void setShape(Shape* newShape) { shape = newShape; }
    void setBrush(Brush* newBrush) { brush = newBrush; }
    void setKey(int newKey) { key = newKey; }
    void select() { selected = true; }
    void deselect() { selected = false; }

    // Operations
    int compare(const Edge* b) const { return key - b->getKey(); }
    void removeMe();
    bool isLeft(const Vertex* v) const;
    float distanceToPoint(const Vertex* point) const;
    float horizontalDistance(const Vertex* point) const;
    // Intercept intercept(const Vec2& position) const;
    float arcLength() const;
    int windingNumber(const Vertex* fillPoint, bool isLeftSide) const;
    bool isClose(const Vec2& query, float scale) const;
    // EdgeCoordinates edgeCoordinates(const Vec2& query) const;
    void split(Vertex* vertex);
    float tangent(float t) const;

    // Drawing
    // void draw(View* view, const Vec2& offset = Vec2(), const std::string& color = "", bool secondPass = false);
    void drawArea(View* view, const Vec2& offset, const DrawOptions& options);

    // Debug
    void print() const;
    int selectType() const;

    static bool isAdjacent(const Edge* e1, const Edge* e2, const Vec2& offset1, const Vec2& offset2);
    static bool adjacentPoints(const Vertex* v1, const Vertex* v2, const Vec2& offset1, const Vec2& offset2);

    static constexpr const char* COLOR = "#500";
    static constexpr const char* SELECTED_COLOR = "#dd0";

private:
    std::unique_ptr<Endpoint> start;
    std::unique_ptr<Endpoint> end;
    Shape* shape;
    bool selected;
    Brush* brush;
    int id;
    int key;

    static int nextId;

    // Helper methods
    float lineDistanceToPoint(const Vertex* point) const;
    float lineHorizontalDistance(const Vertex* point) const;
    //Intercept lineIntercept(const Vec2& position) const;

    struct Intercept {
        std::vector<Vertex*> vertices;
        std::vector<float> tangents;
    };

    struct EdgeCoordinates {
        float u;
        float v;
    };
};

} // namespace ms 