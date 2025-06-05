#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Graph;
class View;
class Primitives;
struct DrawOptions;
struct OrderInfo;

class ProductionRule {
public:
    explicit ProductionRule(
        const std::vector<Graph*>& startGraphs,
        const std::vector<Graph*>& endGraphs
    );
    static ProductionRule* import(const Json& json, Primitives* shape);
    ~ProductionRule() = default;

    const std::vector<Graph*>& getStartGraphs() const { return startGraphs; }
    const std::vector<Graph*>& getEndGraphs() const { return endGraphs; }
    bool isGround() const { return ground; }
    int getId() const { return id; }

private:
    std::vector<Graph*> startGraphs;
    std::vector<Graph*> endGraphs;
    bool ground;
    int id;

    static int nextId;
};

} // namespace ms 