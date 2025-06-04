#pragma once
#include <vector>
#include "../geometry/vec2.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/half_edge.h"

namespace ms {

// class Edge

class TransistorPath {
public:
    struct IndexInfo {
        int index;
        bool isForward;
    };

    static int count;
    static TransistorPath* createNet(const std::vector<Endpoint*>& endpoints,
                                   const std::vector<Line*>& edges,
                                   std::vector<Line*>* lines);

    TransistorPath(const std::vector<IndexInfo>& indices, std::vector<Line*>* lines);

    void setEndpoints(const std::vector<Endpoint*>& endpoints);
    int extendableness() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Line* lineFromIndex(int index);
    IndexInfo indexForEndpoint(Endpoint* endpoint);
    void expandBackward();
    void expandForward();
    void merge(TransistorPath* pathB);

    // Member variables
    std::vector<IndexInfo> indices;
    std::vector<Line*>* lines;
    std::vector<Endpoint*> endpoints;
    std::vector<bool> extendable;
    int id;
};

} // namespace ms 