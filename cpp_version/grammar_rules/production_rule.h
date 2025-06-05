#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Graph;
class View;
class Shape3D;
struct DrawOptions;
struct OrderInfo;

class NetTransition {
public:
    explicit NetTransition(
        const std::vector<Graph*>& startGraphs,
        const std::vector<Graph*>& endGraphs
    );
    static NetTransition* import(const Json& json, Shape3D* shape);
    ~NetTransition() = default;

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