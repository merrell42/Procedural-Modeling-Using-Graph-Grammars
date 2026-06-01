#pragma once
#include <iosfwd>
#include "../geometry/vec2.h"
#include "edge_type.h"
#include "primitives.h"
#include <vector>
#include <memory>
#include <map>
#include <string>
#include "../third_party/json.h"

using Json = nlohmann::json;
using namespace std;

class EdgeType;
class Primitives;

struct HalfEdgeType {
    HalfEdgeType(EdgeType* newEdge = nullptr, bool newIsAtStart = false, const vector<int>& faceIds = {});

    Vec3 dir;
    EdgeType* edge;
    bool isAtStart;
    
    // RuleGenerator support - get ID for half-edge matching.
    string getId() const;
    
    static string oppositeId(const string& id) {
        string opposite(id);
        if (opposite.length() > 0) {
            if (opposite[opposite.length() - 1] == 'S') {
                opposite[opposite.length() - 1] = 'E';
            } else if (opposite[opposite.length() - 1] == 'E') {
                opposite[opposite.length() - 1] = 'S';
            }
        }
        return opposite;
    }
};

class VertexType {
public:
    VertexType();
    ~VertexType() = default;
    static VertexType* import(const Json& json, Primitives* shape);
    static VertexType* binaryDeserialize(std::istream& in, Primitives* shape);

    const vector<HalfEdgeType>& getHalfEdgeTypes() const;
    bool getSpliced() const;
    void setSpliced(bool spliced);

    void addHalfEdge(EdgeType* edge, bool isAtStart);

    // RuleGenerator support.
    int getRuleGeneratorId() const;
    void setRuleGeneratorId(int id);

    // Per-vertex-type bias consumed by the optimizer's cost function. A
    // positive value makes configurations containing this vertex type
    // cheaper (more desirable); a negative value makes them more expensive.
    // Loaded from the `desirability` JSON field on a vertex type, default 0.0.
    double desirability = 0.0;

private:
    vector<HalfEdgeType> halfEdgeTypes;
    bool spliced;
    // RuleGenerator fields.
    int ruleGeneratorId;
};

