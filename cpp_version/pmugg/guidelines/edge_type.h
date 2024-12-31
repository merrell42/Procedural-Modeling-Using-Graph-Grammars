#pragma once
#include "../node/node.h"
#include "../shape/vec2.h"
#include "../decoration/brush.h"
#include <vector>
#include <memory>
#include <string>

namespace ms {

class Edge;
class VertexType;
class Area;

class EdgeType {
public:
    EdgeType(const std::string& name = "");
    ~EdgeType() = default;

    // Core functionality
    void addEdge(Edge* edge);
    void removeEdge(Edge* edge);
    bool isConnected() const;
    void setConnected(bool connected);
    void setBrush(Brush* brush);
    Brush* getBrush() const;
    void setName(const std::string& name);
    std::string getName() const;
    void setStartVertexType(VertexType* vertexType);
    void setEndVertexType(VertexType* vertexType);
    VertexType* getStartVertexType() const;
    VertexType* getEndVertexType() const;
    void setLeftArea(Area* area);
    void setRightArea(Area* area);
    Area* getLeftArea() const;
    Area* getRightArea() const;
    std::vector<Edge*> getEdges() const;
    void print() const;
    Vec2 getDir() const;
    int getId() const { return id; }

private:
    std::string name;
    std::vector<Edge*> edges;
    bool connected;
    Brush* brush;
    VertexType* startVertexType;
    VertexType* endVertexType;
    Area* leftArea;
    Area* rightArea;
    float angle;
    // static int count;  // Add static counter
    int id;           // Add id field
};

} // namespace ms 