#pragma once
#include <vector>
#include <memory>
#include <map>
#include "../geometry/vec3.h"
#include "primitives.h"
#include "../decoration/brush.h"
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

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

    const vector<FaceData>& getFaceData() const { return faceData; }
    const Vec3& getDir() const { return dir; }
    Brush* getBrush() const { return brush; }
    double getAngle() const { return angle; }
    double getEdgeLength() const { return edgeLength; }
    bool getIsRigid() const { return isRigid; }
    bool getSpliced() const { return spliced; }
    bool isDestroyed() const { return destroyed; }
    int getId() const { return id; }

    void setAngle(double newAngle) { angle = newAngle; }
    void setSpliced(bool newSpliced);
    void setMonotonic(bool newMonotonic) { monotonic = newMonotonic; }
    void destroy() { destroyed = true; }

    bool isLoopy() const;
    bool isBoundary() const;
    bool isConnected() const;
    bool singleFragment() const;
    bool splittable() const;
    bool extendable() const;
    double getThickness() const;
    string boundaryString() const;
    int neighboringFace(int initialIndex, bool above) const;

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
    bool destroyed;
    int id;

    static int nextId;
};

} // namespace ms 