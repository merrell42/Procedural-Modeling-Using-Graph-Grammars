#pragma once
#include <vector>
#include <memory>
#include <string>
#include "bound_net.h"
#include "../shapes3D/edge_type3d.h"
#include "../shapes3D/shape3d.h"
#include "primal_object.h"

namespace ms {

class VertexNet;
class EdgeNet;
class BoundNet;

class PrimalEdge : public PrimalObject {
public:
    PrimalEdge(EdgeType3D* edgeType);
    ~PrimalEdge() override = default;

    // Override the copy function
    PrimalObject* copy() const override;

    // Core accessors
    EdgeType3D* getType() const { return type; }
    VertexNet* getBoundary() const { return boundary.empty() ? nullptr : boundary[0]; }
    EdgeNet* getInterior() const { return interior.empty() ? nullptr : interior[0]; }
    int getId() const { return id; }

    // String operations
    std::string boundaryString() const;

    // Debug
    // void print() const;

    // Import/Export
    void import(BoundNet* boundNet, Shape3D* types, const Json& json);
    // Json export() const;

private:
    std::vector<VertexNet*> boundary;
    std::vector<EdgeNet*> interior;
    EdgeType3D* type;
    int id;

    static int nextId;

    friend class BoundNet;  // Allows BoundNet to access private members for connection operations
};

} // namespace ms 