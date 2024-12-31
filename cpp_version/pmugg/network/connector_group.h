#pragma once
#include <vector>
#include <memory>

namespace ms {

class Network;
class Vertex;

class ConnectorGroup {
public:
    ConnectorGroup();
    ~ConnectorGroup() = default;

    // Core functionality
    Network* getNetwork() const;
    const std::vector<Vertex*>& getConnectors() const;
    int getId() const;

    // Network operations
    ConnectorGroup* connectNet(Network* network);
    void addConnector(Vertex* connector, int index);
    void copyConnection(const ConnectorGroup* copy);

    // Import/Export
    // Json export() const;
    void import(const Json& json);

private:
    Network* network;
    std::vector<Vertex*> connectors;
    int id;

    static int nextId;
};

} // namespace ms 