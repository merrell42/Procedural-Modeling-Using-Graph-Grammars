#pragma once
#include <vector>
#include <memory>
#include "primal_object.h"

namespace ms {

class FaceType;
class DualObject;

class PrimalFace : public PrimalObject {
public:
    PrimalFace(FaceType* type, int turns = 0, bool wildcard = false);
    ~PrimalFace() override = default;

    // Override the copy function
    PrimalObject* copy() const override;

    // Core accessors
    FaceType* getType() const { return type; }
    EdgeNet* getBoundary() const { return boundary.empty() ? nullptr : boundary[0]; }
    FaceNet* getInterior() const { return interior.empty() ? nullptr : interior[0]; }
    int getTurns() const { return turns; }
    bool getWildcard() const { return wildcard; }
    int getId() const { return id; }

    // Modifiers
    void setTurns(int newTurns) { turns = newTurns; }
    void setWildcard(bool isWildcard) { wildcard = isWildcard; }

    // Operations
    void computeTurns();
    std::string boundaryString() const;

    // Debug
    void print() const;

    // Import/Export
    void import(BoundNet* boundNet, Shape3D* types, const Json& json);
    // Json export() const;

private:
    std::vector<EdgeNet*> boundary;
    std::vector<FaceNet*> interior;
    FaceType* type;
    int turns;
    bool wildcard;
    int id;

    static int nextId;

    friend class BoundNet;  // Allows BoundNet to access private members for connection operations
};

} // namespace ms 