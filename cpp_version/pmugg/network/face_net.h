#pragma once
#include <vector>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class HalfEdgeNet;
class PrimalFace;
class Network;

class FaceNet {
public:
    FaceNet();
    
    // Getters
    HalfEdgeNet* getOuterComponent() const;
    const std::vector<HalfEdgeNet*>& getInnerComponents() const;
    PrimalFace* getPrimal() const;
    Network* getNetwork() const;
    
    // Setters
    void setPrimal(PrimalFace* primal);
    FaceNet* connectNet(Network* network);
    void connectOuter(const std::vector<HalfEdgeNet*>& halfEdges);
    void makeInner(HalfEdgeNet* halfEdge);
    void copyConnection(const FaceNet* copy);
    
    // Import/Export
    void import(const Json& json);
    // Json export() const;
    
    // Utility functions
    static std::vector<const HalfEdgeNet*> getConnectedHalfEdges(const HalfEdgeNet* start);
    std::vector<const HalfEdgeNet*> getOuterHalfEdges() const;
    std::vector<const HalfEdgeNet*> getInnerHalfEdges() const;
    std::vector<const HalfEdgeNet*> getHalfEdges() const;
    // void highlight(View* view);
    void getHalfEdgeSet();
    void merge(FaceNet* faceB);
    void replaceHalfEdge(HalfEdgeNet* a, HalfEdgeNet* b, bool force);
    bool isLoopy() const;
    bool inNetwork() const;
    void print();

private:
    HalfEdgeNet* outerComponent;
    std::vector<HalfEdgeNet*> innerComponents;
    PrimalFace* primal;
    Network* network;
    int id;
};

} // namespace ms 