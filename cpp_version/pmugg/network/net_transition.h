#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class Network;
class View;
class Shape3D;
struct DrawOptions;
struct OrderInfo;

class NetTransition {
public:
    explicit NetTransition(
        const std::vector<Network*>& startNetworks,
        const std::vector<Network*>& endNetworks
    );
    static NetTransition* import(const Json& json, Shape3D* shape);
    ~NetTransition() = default;

    const std::vector<Network*>& getStartNetworks() const { return startNetworks; }
    const std::vector<Network*>& getEndNetworks() const { return endNetworks; }
    bool isGround() const { return ground; }
    int getId() const { return id; }

private:
    std::vector<Network*> startNetworks;
    std::vector<Network*> endNetworks;
    bool ground;
    int id;

    static int nextId;
};

} // namespace ms 