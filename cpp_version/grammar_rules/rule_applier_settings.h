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
    std::string type;
    int vertexId;
};

class NetTransistorSettings {
public:
    NetTransistorSettings(Vec3 lower, Vec3 upper) : lower(lower), upper(upper) {};

    VertexPlacement* getVertex(int id) { return vertexPlacements[id].get(); }
    EdgePlacement* getEdge(int id) { return edgePlacements[id].get(); }
    FacePlacement* getFace(int id) { return facePlacements[id].get(); }

    void setVertex(int id, std::unique_ptr<VertexPlacement> vPlace);
    void setEdge(int id, std::unique_ptr<EdgePlacement> ePlace);
    void setFace(int id, std::unique_ptr<FacePlacement> fPlace);

    void addToOrder(int id, const std::string& type, int vertexId);
    int createFace(const Vec3& normal);
    void mergeFace(int idA, int idB);
    int findBasisOrder(const int basisId);

    // Member variables
    int newFaceCounter = -1;
    std::unordered_map<int, std::unique_ptr<VertexPlacement>> vertexPlacements;
    std::unordered_map<int, std::unique_ptr<EdgePlacement>> edgePlacements;
    std::unordered_map<int, std::unique_ptr<FacePlacement>> facePlacements;
    std::vector<int> basisIds;
    Vec3 lower;
    Vec3 upper;
    std::unordered_map<int, int> uniqueFaceMap;
    std::vector<int> orderIds;
    std::vector<OrderInfo> orderInfo;

    static constexpr double defaultLengthMin = 0.2;
    static constexpr double defaultLengthMax = 4;
};

} // namespace ms 