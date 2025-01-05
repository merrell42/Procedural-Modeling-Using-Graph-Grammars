#pragma once
#include <vector>
#include "../shape/vec2.h"
#include "../graph_drawing/line.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/endpoint.h"

namespace ms {

// class Edge

class TransistorPath {
public:
    struct IndexInfo {
        int index;
        bool isForward;
    };

    // Static members
    static int count;
    static TransistorPath* createNet(const std::vector<Endpoint*>& endpoints,
                                   const std::vector<Line*>& edges,
                                   const std::vector<Line*>& lines);
    //static TransistorPath* create(const std::vector<Endpoint*>& endpoints,
    //                            const std::vector<Line*>& edges,
    //                            const std::vector<Line*>& lines);

    // Constructor
    TransistorPath(const std::vector<IndexInfo>& indices, const std::vector<Line*>& lines);

    // Member functions
    void setEndpoints(const std::vector<Endpoint*>& endpoints);
    int extendableness() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Line* lineFromIndex(int index);
    IndexInfo indexForEndpoint(Endpoint* endpoint);
    void expandBackward();
    void expandForward();
    void merge(TransistorPath* pathB);
    // void highlight(Context* context, const std::function<Vec2(const Vec2&)>& convertToScreen);
    /// void print();

    // Member variables
    std::vector<IndexInfo> indices;
    std::vector<Line*> lines;
    std::vector<Endpoint*> endpoints;
    std::vector<bool> extendable;
    int id;
};

} // namespace ms 