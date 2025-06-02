#include "pch.h"
#include "net_transition.h"
#include "network.h"

namespace ms {

int NetTransition::nextId = 0;

NetTransition::NetTransition(
    const std::vector<Network*>& startNetworks,
    const std::vector<Network*>& endNetworks
) : startNetworks(startNetworks),
    endNetworks(endNetworks),
    ground(false),
    id(nextId++) {}

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

    int vertexSize = startNetworks[0]->getBVertices().size();
    int halfEdgeSize = startNetworks[0]->getBHalfEdges().size();
    int faceSize = startNetworks[0]->getBFaces().size();
    for (int i = 1; i < startNetworks.size(); i++) {
        if (startNetworks[i]->getBVertices().size() != vertexSize) {
            throw std::runtime_error("Boundary vertex mismatch");
        }
    }

    auto* result = new NetTransition(startNetworks, endNetworks);
    result->ground = json["ground"];
    return result;
}

} // namespace ms 