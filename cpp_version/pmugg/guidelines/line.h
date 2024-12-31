#pragma once
#include "../shape/vec2.h"
#include "../node/node.h"
#include <vector>
#include <memory>

namespace ms {

class Stats;
class EdgeType;
class LineSegment;
class RingInstance;
class Endpoint;
class Vertex;
class Face;

class Line {
public:
    Line(Stats* stats, EdgeType* edgeType);
    ~Line() = default;

    // Core functionality
    Node* getNode() const;
    std::vector<LineSegment*> getSegments() const;
    std::vector<RingInstance*> getRingInstances() const;
    std::vector<Endpoint*> getEndpoints() const;
    LineSegment* getSegment(bool isAtStart = false) const;
    LineState* getState(bool isAtStart = false) const;
    EdgeType* getEdgeType() const;
    float getCost() const;
    void setCost(float cost);
    // const TileSeeds& getTileSeeds() const;

    // Operations
    Line* copy() const;
    void addSegments(const std::vector<LineSegment*>& segments, bool atStart = false);
    void addEndpoint(Endpoint* endpoint, int index);
    bool isRigid() const;
    // SplitResult split();
    Brush* getBrush() const;
    void fillFromEndpoints(bool addToModel = false);
    void moveToEndpoints();
    bool findIntersections();
    float getAngle() const;
    Vec2 getDir() const;
    std::vector<Face*> getFaceData() const;

    // Drawing
    void draw(View* view, const Vec2& offset = Vec2());
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void print() const;

    // Static methods
    static Line* createFromPositions(const std::vector<Vec2>& positions, float angle0, 
                                   EdgeType* edgeType, Stats* stats);
    static Line* createFromEndpoints(Stats* stats, const std::vector<Endpoint*>& endpoints);
    static VertexType* getVertexType(EdgeType* edgeType, Shape3D* shape = nullptr);
    // static SplitResult fullSplit(float s);

    // Constants
    static constexpr float RENDER_WIDTH_3D = 0.5f;
    static constexpr float BACKTRACK = 0.1f;
    static constexpr float STATE_LENGTH = 1.0f;

private:
    EdgeType* edgeType;
    std::unique_ptr<Node> node;
    // TileSeeds tileSeeds;

    struct TileSeeds {
        int base;
        int increment;
    };

    struct SplitResult {
        std::vector<Line*> lines;
        std::vector<Endpoint*> nextEndpoints;
    };

    static std::unordered_map<int, VertexType*> splitVertexTypes;
};

} // namespace ms 