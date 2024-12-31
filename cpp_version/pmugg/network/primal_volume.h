#pragma once
#include <vector>
#include <memory>
#include <string>
#include "primal_object.h"

namespace ms {

class DualObject;
class BoundNet;
class Shape3D;

class PrimalVolume : public PrimalObject {
public:
    PrimalVolume();
    ~PrimalVolume() override = default;

    // Override the copy function
    PrimalObject* copy() const override;

    // Core accessors
    int getType() const { return type; }
    DualObject* getBoundary() const { return boundary.empty() ? nullptr : boundary[0]; }
    DualObject* getInterior() const { return interior.empty() ? nullptr : interior[0]; }
    int getId() const { return id; }

    // Volume operations
    int getTurns() const { return 0; }  // Turns don't matter for volumes
    void setTurns(int) {}               // No-op for volumes
    std::string boundaryString() const;

    // Debug
    void print() const;

    // Import/Export
    void import(BoundNet* boundNet, Shape3D* types, const Json& json);
    // Json export() const;

private:
    std::vector<DualObject*> boundary;
    std::vector<DualObject*> interior;
    int type;
    int id;

    static int nextId;

    friend class BoundNet;  // Allows BoundNet to access private members for connection operations
};

} // namespace ms 