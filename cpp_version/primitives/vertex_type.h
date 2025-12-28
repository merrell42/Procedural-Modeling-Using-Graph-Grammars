#pragma once
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
};

// Connection between an edge and a vertex for RuleGenerator.
struct VertexConnection {
    EdgeType* edge;
    bool isAtStart;
    
    VertexConnection(EdgeType* e, bool atStart) : edge(e), isAtStart(atStart) {}
    
    string getId() const;
    
    static string oppositeId(const string& id) {
        string opposite(id);
        if (opposite.length() > 0) {
            if (opposite[opposite.length() - 1] == 'S') {
                opposite[opposite.length() - 1] = 'E';
            }
            else if (opposite[opposite.length() - 1] == 'E') {
                opposite[opposite.length() - 1] = 'S';
            }
        }
        return opposite;
    }
    
    bool Equals(const VertexConnection& other) const {
        return edge == other.edge && isAtStart == other.isAtStart;
    }
};

class VertexType {
public:
    VertexType();
    ~VertexType() = default;
    static VertexType* import(const Json& json, Primitives* shape);
    static VertexType* importRuleGenerator(const Json& json, const map<string, EdgeType*>& eTypes);

    const vector<HalfEdgeType>& getHalfEdgeTypes() const;
    bool getSpliced() const;
    void setSpliced(bool spliced);

    void addHalfEdge(EdgeType* edge, bool isAtStart);

    // RuleGenerator support.
    const vector<VertexConnection>& getConnections() const;
    int getRuleGeneratorId() const;
    void setRuleGeneratorId(int id);

private:
    vector<HalfEdgeType> halfEdgeTypes;
    bool spliced;
    // RuleGenerator fields.
    vector<VertexConnection> connections;
    int ruleGeneratorId;
};

