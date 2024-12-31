#pragma once
#include "../node/node.h"
#include "../shape/vec2.h"
#include "../guidelines/vertex_type.h"
#include <vector>
#include <memory>

namespace ms {

class Stats;
class VertexState;
class Endpoint;
class LineState;
class Face;

class Vertex {
public:
    Vertex(Stats* stats, const Vec2& position);
    ~Vertex() = default;

    // Core functionality
    // Node* getNode() const;
    std::vector<Endpoint*> getEndpoints() const;
    Endpoint* getEndpoint(int index) const;
    VertexState* getState() const;
    std::vector<LineState*> getLineStates() const;
    Vec2 getPosition() const;
    int getId() const;

    // State management
    void addLineState(LineState* lineState, int index);
    void removeLineStates();
    void transferLineStates(Vertex* vertexB);
    void resolveEndpoints();

    // Position operations
    void setPosition(const Vec2& position);
    bool isMutable() const;
    bool isMoveable() const;

    // Merging operations
    void merge(Vertex* vertexB, Endpoint* connectingEndpoint = nullptr);
    bool hasConflict() const;

    // Drawing
    // void fillHighlight(RenderData& renderData);
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    // void fillRenderData(RenderData& renderData);
    void draw(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void print() const;

    // Static methods
    static Vertex* createWithState(Stats* stats, const Vec2& position, float angle, 
                                 float scale, VertexType* type, bool primal = false);
    static bool compare(Endpoint* endpointA, Endpoint* endpointB);
    Node* getNode() const;

    // Static members
    static bool vertexMergingEnabled;
    static constexpr float closeVertexThreshold = 0.5f;
    static constexpr float closeVertexThreshold2 = closeVertexThreshold * closeVertexThreshold;

private:
    std::unique_ptr<Node> node;
    // TileSeeds tileSeeds;
    int imageSeed;

    struct TileSeeds {
        int base;
        int increment;
    };

    // Helper methods
    void updateStats();
    void onEndpointsChanged();
    void onDestroy();
};

} // namespace ms 