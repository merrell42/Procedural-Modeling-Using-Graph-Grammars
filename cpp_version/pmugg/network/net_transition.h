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
    explicit NetTransition(const std::vector<Network*>& networks);
    ~NetTransition() = default;

    // Core functionality
    const std::vector<Network*>& getNetworks() const { return networks; }
    bool isGround() const { return ground; }
    int getId() const { return id; }

    // Network operations
    void addNetwork(Network* network);

    // Drawing
    // void highlight(View* view, const DrawOptions& options = {});
    // void print() const;

    // Import/Export
    static NetTransition* import(const Json& json, Shape3D* shape);
    // Json export() const;

private:
    std::vector<Network*> networks;
    bool ground;
    int id;

    static int nextId;
};

} // namespace ms 