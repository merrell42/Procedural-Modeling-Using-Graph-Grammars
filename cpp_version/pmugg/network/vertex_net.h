#pragma once
#include <vector>
#include <memory>
// #include "primal_vertex.h"
#include "../third_party/json.h"

using Json = nlohmann::json;


namespace ms {

class Network;
class HalfEdgeNet;
class ConnectorGroup;
class View;
struct DrawOptions;

class VertexNet {
public:
    VertexNet();
    ~VertexNet() = default;

    // Core accessors
    const std::vector<HalfEdgeNet*>& getHalfEdges() const { return halfEdges; }
    // PrimalVertex* getPrimal() const { return primal; }
    Network* getNetwork() const { return network; }
    ConnectorGroup* getGroup() const { return group; }
    int getId() const { return id; }

    // Network operations
    // void setPrimal(PrimalVertex* primal);
    VertexNet* connectNet(Network* network);
    void setHalfEdge(HalfEdgeNet* halfEdge, int index);
    void copyConnection(const VertexNet* copy);

    // Group operations
    void setGroup(ConnectorGroup* newGroup) { group = newGroup; }

    // Validation
    bool inNetwork() const;

    // Drawing
    /*void highlight(View* view, const DrawOptions& options = {});
    void print() const;*/
    bool requiresShapeView() const { return true; }

    // Import/Export
    // Json export() const;
    void import(const Json& json);

private:
    std::vector<HalfEdgeNet*> halfEdges;
    // PrimalVertex* primal;
    Network* network;
    ConnectorGroup* group;
    int id;

    static int nextId;
};

} // namespace ms 