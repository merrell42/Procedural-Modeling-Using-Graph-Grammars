#pragma once
#include <vector>
#include "../placements/vertex_placement.h"
#include "../placements/edge_placement.h"
#include "../placements/face_placement.h"
#include "../graph_morphism/morphism_path.h"
#include "../util/timer.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

class VertexPlacement;
class EdgePlacement;
class FacePlacement;

struct OrderInfo {
    string type;
    int vertexId;
};

class RuleApplierSettings {
public:
    RuleApplierSettings(Vec3 lower, Vec3 upper) : lower(lower), upper(upper) {};

    VertexPlacement* getVertex(int id);
    EdgePlacement* getEdge(int id);
    FacePlacement* getFace(int id);

    void addToOrder(int id, const string& type, int vertexId);
    int createFace(const Vec3& normal);
    void mergeFace(int idA, int idB);
    int findBasisOrder(const int basisId);

    // Member variables.
    int newFaceCounter = -1;

    // Maps an ID to a placement.
    map<int, unique_ptr<VertexPlacement>> vertexPlacements;
    map<int, unique_ptr<EdgePlacement>> edgePlacements;
    map<int, unique_ptr<FacePlacement>> facePlacements;

    // A list of face IDs that act as the basis for the solution space.
    vector<int> basisIds;

    // The upper and lower bounds of the solution space.
    Vec3 lower;
    Vec3 upper;

    // Coplanar faces are merged into the same face. This maps each face to a
    // unique ID where coplanar faces have the same ID.
    map<int, int> uniqueFaceMap;

    // The order in which each placement is set to a fixed position.
    vector<int> orderIds;
    vector<OrderInfo> orderInfo;

    static constexpr double defaultLengthMin = 0.2;
    static constexpr double defaultLengthMax = 4;
};

