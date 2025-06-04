#pragma once
#include <vector>
#include "../geometry/vec3.h"
#include "../util/range.h"

namespace ms {

class Face;
class FacePlacement;
class NetTransistorSettings;

struct ChangeMB {
    double m;
    double b;
};

struct FixedFace {
    Face* faceA;
    FacePlacement* fPlace;
    double d;
};

class FacePlacement {
public:
    FacePlacement(const Vec3& normal, int id, NetTransistorSettings* settings, Face* face);

    Face* getFace() const;
    double getD() const;
    bool isFree() const;
    const Vec3& getNormal() const;
    void setFixed(bool fixed);
    bool getFixed() const;
    void addVertexId(int id);
    void addFixedNeighbor(Face* neighbor);
    bool coplanar(FacePlacement* fPlaceB) const;
    void constrain(bool addBasis, int vertexId);
    void setD(double d);
    bool setFromVertex(int vertexId);
    void makeFixed(const FixedFace& faceA);
    Range getRange(int vertexId = -1);
    ChangeMB getChangeMB() const;

    std::vector<int> vertexIds;
    int id;

private:
    Vec3 normal;
    Face* face;
    NetTransistorSettings* settings;
    bool free{true};
    std::vector<Face*> fixedNeighbors;
    double d{0.0};
    bool fixed{false};
    double slope{0.0};
    double value{0.0};
};

} // namespace ms 