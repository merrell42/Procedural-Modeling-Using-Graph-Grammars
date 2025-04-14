#include "pch.h"
#include "net_transition.h"
#include "network.h"
// #include "view.h"
// #include "types.h"
// #include "graph.h"

namespace ms {

int NetTransition::nextId = 0;

NetTransition::NetTransition(
    const std::vector<Network*>& startNetworks,
    const std::vector<Network*>& endNetworks
) : startNetworks(startNetworks),
    endNetworks(endNetworks),
    ground(false),
    id(nextId++) {}

//void NetTransition::highlight(View* view, const DrawOptions& options) {
//    auto h = view->getViewport().height;
//    auto size = Graph::HIGHLIGHTED_SIZE;
//
//    DrawOptions options0 = options;
//    DrawOptions options1 = options;
//
//    Vec2 offset = options.offset.value_or(Vec2(20, view->getCanvas().height - size - 20));
//    
//    options0.rect = {offset.x, offset.x + size, offset.y, offset.y + size, 0, 100};
//    options1.rect = {offset.x + size, offset.x + 2 * size, offset.y, offset.y + size, 0, 100};
//
//    networks[0]->draw(view, options0);
//    networks[1]->draw(view, options1);
//}
//
//void NetTransition::print() const {
//    ms::highlight(this);
//}

NetTransition* NetTransition::import(const Json& json, Shape3D* shape) {
    std::vector<Network*> startNetworks;
    std::vector<Network*> endNetworks;
    for (size_t index = 0; index < json["n"].size(); ++index) {
        const auto& networkJson = json["n"][index];
        startNetworks.push_back(Network::import(networkJson, shape));

        // End networks are the same as start networks except the splices are removed.
        // This could be done by copying startNetwork rather than importing it again.
        const auto endNetwork = Network::import(networkJson, shape);
        endNetwork->removeSplices();
        endNetworks.push_back(endNetwork);
    }

    auto* result = new NetTransition(startNetworks, endNetworks);
    result->ground = json["ground"];
    return result;
}

//Json NetTransition::export() const {
//    Json json;
//    json["n"] = Json::array();
//    
//    for (auto* network : networks) {
//        json["n"].push_back(network->export());
//    }
//    
//    json["ground"] = ground;
//    return json;
//}

} // namespace ms 