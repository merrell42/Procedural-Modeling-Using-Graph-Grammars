#pragma once
#include "../shape/vec2.h"
#include <vector>
#include <memory>

namespace ms {

class Face;
class Cell;
class Endpoint;

class FaceConnection {
public:
    FaceConnection(Face* face = nullptr, Endpoint* start = nullptr, Endpoint* end = nullptr);
    ~FaceConnection() = default;

    // Core functionality
    void addCell(Cell* cell);
    void removeCell(Cell* cell);
    void setFace(Face* face);
    Face* getFace() const;
    std::vector<Cell*> getCells() const;
    std::vector<Endpoint*> getEndpoints() const;
    bool isLeft(Endpoint* endpoint) const;

    // Cell operations
    void addCells();
    void removeCells();
    bool intersectsState(const LineState& lineState) const;

    // Drawing operations
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void draw(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen, bool highlighted = false);
    void print() const;

private:
    Face* face;
    std::vector<Cell*> cells;
    std::vector<Endpoint*> endpoints;
    std::vector<Vec2> coordinates;
};

} // namespace ms 