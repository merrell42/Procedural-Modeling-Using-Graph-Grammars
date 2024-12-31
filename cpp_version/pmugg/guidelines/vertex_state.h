#pragma once
#include "../shape/vec2.h"
#include "../node/node.h"
#include <vector>
#include <memory>

namespace ms {

class Stats;
class Vertex;
class Endpoint;
class RingInstance;
class VertexType;

class VertexState {
public:
    VertexState(Stats* stats, Vertex* vertex, float angle, float scale, VertexType* type, bool primal = false);
    ~VertexState() = default;

    // Core functionality
    Node* getNode() const;
    Vertex* getVertex() const;
    std::vector<Endpoint*> getEndpoints() const;
    Endpoint* getEndpoint(int index) const;
    std::vector<RingInstance*> getRingInstances() const;
    VertexType* getType() const;
    float getAngle() const;
    float getScale() const;

    // State management
    void resolveEndpoints();
    void resolveEndpointsNet();
    void resolveEndpointsGraph();
    void remove();
    void removeLeaves();
    int endpointIndex(Endpoint* endpoint) const;
    void addEndpoint(Endpoint* endpoint, int index);

    // Drawing
    void highlight(void* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    void print() const;

private:
    std::unique_ptr<Node> node;
    float angle;
    float scale;
    VertexType* type;

    // Helper methods
    Endpoint* createEndpointNet(const Connection& connection, int vertexIndex, int faceIndex);
    Endpoint* createEndpointGraph(int index);
};

} // namespace ms 