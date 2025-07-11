#pragma once
#include <vector>
#include <memory>
#include <map>
#include "../geometry/vec3.h"
#include "primitives.h"
#include "../graph/edge_settings.h"
#include "../third_party/json.h"

using Json = nlohmann::json;

class FaceType;
class Primitives;

// A face type and a boolean for if the face is on the right or left side of the edge.
struct FaceData {
    FaceType* type;
    bool onRight;
};

// Represents an edge type. Edges with the same type are locally similar.
// Contains a direction and an array of face types.
class EdgeType {
public:
    EdgeType(const vector<FaceData>& faceData, const Vec3& dir, 
               const map<string, bool>& options = {});
    ~EdgeType() = default;

    const vector<FaceData>& getFaceData() const;
    const Vec3& getDir() const;
    EdgeSettings* getEdgeSettings() const;
    bool getIsRigid() const;
    bool getSpliced() const;
    int getId() const;

    void setSpliced(bool newSpliced);
    bool extendable() const;

    static EdgeType* import(const Json& json, Primitives* shape);

    vector<FaceData> faceData;

private:
    Vec3 dir;
    EdgeSettings* edgeSettings;
    bool isRigid;
    bool isRigidTiled;
    bool spliced;
    int id;

    static int nextId;
};

