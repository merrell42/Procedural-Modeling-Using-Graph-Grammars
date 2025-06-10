#pragma once
#include <vector>
#include <memory>
#include <map>
#include "../geometry/vec3.h"
#include "primitives.h"
#include "../decoration/brush.h"
#include "../third_party/json.h"

using Json = nlohmann::json;

class FaceType;
class Primitives;

struct FaceData {
    FaceType* type;
    bool onRight;
};

class EdgeType {
public:
    EdgeType(const vector<FaceData>& faceData, const Vec3& dir, 
               const map<string, bool>& options = {});
    ~EdgeType() = default;

    const vector<FaceData>& getFaceData() const;
    const Vec3& getDir() const;
    Brush* getBrush() const;
    double getAngle() const;
    double getEdgeLength() const;
    bool getIsRigid() const;
    bool getSpliced() const;
    int getId() const;

    void setSpliced(bool newSpliced);
    bool extendable() const;

    static EdgeType* import(const Json& json, Primitives* shape);

    vector<FaceData> faceData;

private:
    Vec3 dir;
    Brush* brush;
    double angle;
    double edgeLength;
    Vec3* offset;
    bool isRigid;
    bool isRigidTiled;
    bool monotonic;
    bool spliced;
    int id;

    static int nextId;
};

