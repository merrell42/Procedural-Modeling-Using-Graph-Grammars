#pragma once
#include <vector>
#include <memory>
#include <map>
#include "../shape/vec3.h"
#include "../shapes3D/shape3d.h"
#include "../decoration/brush.h"
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class FaceType3D;
class Shape3D;

struct FaceData {
    FaceType3D* type;
    bool onRight;
};

class EdgeType3D {
public:
    EdgeType3D(const std::vector<FaceData>& faceData, const Vec3& dir, 
               const std::map<std::string, bool>& options = {});
    ~EdgeType3D() = default;

    // Core accessors
    const std::vector<FaceData>& getFaceData() const { return faceData; }
    const Vec3& getDir() const { return dir; }
    Brush* getBrush() const { return brush; }
    float getAngle() const { return angle; }
    float getEdgeLength() const { return edgeLength; }
    bool getIsRigid() const { return isRigid; }
    bool getSpliced() const { return spliced; }
    bool isDestroyed() const { return destroyed; }
    int getId() const { return id; }

    // Modifiers
    void setAngle(float newAngle) { angle = newAngle; }
    void setSpliced(bool newSpliced);
    void setMonotonic(bool newMonotonic) { monotonic = newMonotonic; }
    void destroy() { destroyed = true; }

    // Operations
    bool is3D() const { return false; }  // TODO: Fix this
    bool isLoopy() const;
    bool isBoundary() const;
    bool isConnected() const;
    bool singleFragment() const;
    bool splittable() const;
    bool extendable() const;
    float getThickness() const;
    std::string boundaryString() const;
    int neighboringFace(int initialIndex, bool above) const;

    // Area operations
    std::string getLeftArea() const;
    std::string getRightArea() const;

    // Import/Export
    // Json export(const Types& types) const;
    static EdgeType3D* import(const Json& json, Shape3D* shape);
    static EdgeType3D* partialImport(const Json& json, const std::vector<FaceType3D*>& faceTypes);

private:
    std::vector<FaceData> faceData;
    Vec3 dir;
    Brush* brush;
    float angle;
    float edgeLength;
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