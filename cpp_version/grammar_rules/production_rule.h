#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;
using namespace std;

class Graph;
class View;
class Primitives;
struct DrawOptions;
struct OrderInfo;

// A production rule consists of a set of graph that have the same
// graph boundary and can be switched out for each other.
class ProductionRule {
public:
    explicit ProductionRule(
        const vector<Graph*>& startGraphs,
        const vector<Graph*>& endGraphs
    );
    static ProductionRule* import(const Json& json, Primitives* shape);
    ~ProductionRule();

    // End graphs are the same as start graphs except the splices are removed.
    // The splices are not needed when creating the end graphs.
    const vector<Graph*>& getStartGraphs() const;
    const vector<Graph*>& getEndGraphs() const;
    bool isGround() const;
    int getId() const;

private:
    vector<Graph*> startGraphs;
    vector<Graph*> endGraphs;
    bool ground;
    int id;

    static int nextId;
};

