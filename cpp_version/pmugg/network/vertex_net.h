#pragma once
#include <vector>
#include <memory>
// #include "primal_vertex.h"
#include "../guidelines/vertex_type.h"
#include "../third_party/json.h"
#include <iostream>

using Json = nlohmann::json;


namespace ms {

class Network;
class HalfEdgeNet;
class ConnectorGroup;
class View;
class VertexType;
struct DrawOptions;

class VertexNet {
public:
    VertexNet();
    ~VertexNet() = default;

    // Core accessors
    const std::vector<HalfEdgeNet*>& getHalfEdges() const { return halfEdges; }
    VertexType* getType() {
        if (kind == "e") {
            std::cout << "Edgetype" << std::endl;
        }
        return type;
    }
    void setType(VertexType* type_) { type = type_; }
    // PrimalVertex* getPrimal() const { return primal; }
    Network* getNetwork() const { return network; }
    ConnectorGroup* getGroup() const { return group; }
    int getId() const { return id; }
    int connectorIndex() const;

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


    // ******** TODO: Replace with edgeType ****
    std::string kind;
private:
    std::vector<HalfEdgeNet*> halfEdges;
    // PrimalVertex* primal;
    VertexType* type;
    Network* network;
    ConnectorGroup* group;
    int id;

    static int nextId;
};

} // namespace ms 