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


namespace ms {

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

    VertexPlacement* getVertex(int id) { return vertexPlacements[id].get(); }
    EdgePlacement* getEdge(int id) { return edgePlacements[id].get(); }
    FacePlacement* getFace(int id) { return facePlacements[id].get(); }

    void setVertex(int id, unique_ptr<VertexPlacement> vPlace);
    void setEdge(int id, unique_ptr<EdgePlacement> ePlace);
    void setFace(int id, unique_ptr<FacePlacement> fPlace);

    void addToOrder(int id, const string& type, int vertexId);
    int createFace(const Vec3& normal);
    void mergeFace(int idA, int idB);
    int findBasisOrder(const int basisId);

    // Member variables
    int newFaceCounter = -1;
    unordered_map<int, unique_ptr<VertexPlacement>> vertexPlacements;
    unordered_map<int, unique_ptr<EdgePlacement>> edgePlacements;
    unordered_map<int, unique_ptr<FacePlacement>> facePlacements;
    vector<int> basisIds;
    Vec3 lower;
    Vec3 upper;
    unordered_map<int, int> uniqueFaceMap;
    vector<int> orderIds;
    vector<OrderInfo> orderInfo;

    static constexpr double defaultLengthMin = 0.2;
    static constexpr double defaultLengthMax = 4;
};

} // namespace ms 