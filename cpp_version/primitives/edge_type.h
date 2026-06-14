#pragma once
#include <vector>
#include <memory>
#include <string>
#include <iosfwd>
#include <atomic>
#include "../geometry/vec3.h"
#include "primitives.h"
#include "../graph/edge_settings.h"
#include "../third_party/json.h"

using Json = nlohmann::json;
using namespace std;

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
    EdgeType(const vector<FaceData>& faceData, const Vec3& dir, bool isRigid = false);
    ~EdgeType() = default;
    static EdgeType* import(const Json& json, Primitives* shape);
    static EdgeType* binaryDeserialize(std::istream& in, Primitives* shape);
    Json exportJson(const Primitives* shape) const;

    const vector<FaceData>& getFaceData() const;
    const Vec3& getDir() const;
    double getAngle() const;
    EdgeSettings* getEdgeSettings() const;
    bool getIsRigid() const;
    bool getSpliced() const;
    int getId() const;

    // RuleGenerator support.
    const string& getRuleGeneratorId() const;
    void setRuleGeneratorId(const string& id);

    void setSpliced(bool newSpliced);
    bool extendable() const;

private:
    vector<FaceData> faceData;
    Vec3 dir;
    EdgeSettings* edgeSettings;
    bool isRigid;
    bool spliced;
    int id;
    // RuleGenerator string id.
    string ruleGeneratorId;

    static std::atomic<int> nextId;
};

