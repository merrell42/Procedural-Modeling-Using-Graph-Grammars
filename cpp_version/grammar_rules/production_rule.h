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

class NetTransition {
public:
    explicit NetTransition(
        const vector<Graph*>& startGraphs,
        const vector<Graph*>& endGraphs
    );
    static NetTransition* import(const Json& json, Primitives* shape);
    ~NetTransition() = default;

    const vector<Graph*>& getStartGraphs() const { return startGraphs; }
    const vector<Graph*>& getEndGraphs() const { return endGraphs; }
    bool isGround() const { return ground; }
    int getId() const { return id; }

private:
    vector<Graph*> startGraphs;
    vector<Graph*> endGraphs;
    bool ground;
    int id;

    static int nextId;
};

