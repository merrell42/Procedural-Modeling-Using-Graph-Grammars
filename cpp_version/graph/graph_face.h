#pragma once
#include <vector>
#include "../third_party/json.h"
#include "../primitives/face_type.h"

using Json = nlohmann::json;

namespace ms {

class HalfEdgeNet;
class Network;
class FaceType3D;

class FaceNet {
public:
    FaceNet();
    void import(const Json& json);

    HalfEdgeNet* getOuterComponent() const;
    const std::vector<HalfEdgeNet*>& getInnerComponents() const;
    Network* getNetwork() const;
    FaceType3D* getType() const { return type; }
    
    FaceNet* connectNet(Network* network);
    void connectOuter(const std::vector<HalfEdgeNet*>& halfEdges);
    void makeInner(HalfEdgeNet* halfEdge);
    void copyConnection(const FaceNet* copy);
    void setType(FaceType3D* type_) { type = type_;}
    
    static std::vector<HalfEdgeNet*> getConnectedHalfEdges(HalfEdgeNet* start);
    std::vector<HalfEdgeNet*> getOuterHalfEdges() const;
    std::vector<HalfEdgeNet*> getInnerHalfEdges() const;
    std::vector<HalfEdgeNet*> getHalfEdges() const;
    void replaceHalfEdge(HalfEdgeNet* a, HalfEdgeNet* b, bool force);
    bool isLoopy() const;
    bool inNetwork() const;

private:
    HalfEdgeNet* outerComponent;
    std::vector<HalfEdgeNet*> innerComponents;
    FaceType3D* type;
    Network* network;
    int id;
};

} // namespace ms 