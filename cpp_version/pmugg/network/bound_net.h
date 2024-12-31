#pragma once
#include <vector>
#include <memory>
#include "../third_party/json.h"
#include "network.h"
#include "../shapes3D/shape3d.h"

using Json = nlohmann::json;

namespace ms {

class Network;
class Shape3D;
class View;
class FaceNet;

class BoundNet {
public:
    BoundNet(Network* interior, Network* boundary, Shape3D* shape3D = nullptr);
    ~BoundNet() = default;

    // Core functionality
    Network* getBoundary() const;
    Network* getInterior() const;
    Shape3D* getShape3D() const;
    int getId() const;
    std::vector<void*> getConnectors();

    // Operations
    BoundNet* copy() const;
    // BoundNet* removeSplices() const;
    void recomputeTurns();
    std::vector<FaceNet*> getOuterFaces() const;

    // Drawing
    /*void highlight(View* view, const DrawOptions& options = {});
    void draw(View* view, const DrawOptions& options = {});
    void print() const;*/

    // Static operations
    static void connectBoundary(void* primal, void* boundary);
    static void connectInterior(void* primal, void* interior);

    // Import/Export
    static BoundNet* import(const Json& json, Shape3D* shape3D);

private:
    Network* interior;
    Network* boundary;
    Shape3D* shape3D;
    std::vector<void*> cachedConnectors;
    int id;

    static int nextId;
};

} // namespace ms 