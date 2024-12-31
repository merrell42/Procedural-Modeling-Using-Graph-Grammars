#pragma once
#include <vector>
#include <memory>
#include "primal_object.h"

namespace ms {

class VertexType;
class DualObject;
class VertexNet;
class BoundNet;
class Types;

class PrimalVertex : public PrimalObject {
public:
    PrimalVertex();
    ~PrimalVertex() override = default;

    // Override the copy function
    PrimalObject* copy() const override;

    // Core accessors
    VertexType* getType() const { return type; }
    VertexNet* getBoundary() const { return boundary.empty() ? nullptr : boundary[0]; }
    VertexNet* getInterior() const { return interior.empty() ? nullptr : interior[0]; }
    DualObject* getConnection() const { return connection; }
    int getId() const { return id; }

    // Edge operations
    DualObject* interiorEdge() const;

    // Debug
    void print() const;

    // Import/Export
    void import(BoundNet* boundNet, Types* types, const Json& json);
    // Json export() const;

    // View requirements
    bool requiresShapeView() const { return true; }

private:
    std::vector<VertexNet*> boundary;
    std::vector<VertexNet*> interior;
    VertexType* type;
    DualObject* connection;  // For fixing order with partial imports
    int id;

    static int nextId;

    friend class BoundNet;  // Allows BoundNet to access private members for connection operations
};

} // namespace ms 