#pragma once
#include <vector>
#include "../geometry/vec2.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/half_edge.h"

namespace ms {

// class Edge

class MorphismPath {
public:
    struct IndexInfo {
        int index;
        bool isForward;
    };

    static int count;
    static MorphismPath* createPath(const std::vector<HalfEdge*>& halfedges,
                                   const std::vector<Edge*>& edges,
                                   std::vector<Edge*>* lines);

    MorphismPath(const std::vector<IndexInfo>& indices, std::vector<Edge*>* edges);

    void setHalfEdges(const std::vector<HalfEdge*>& halfedges);
    int extendableness() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Edge* edgeFromIndex(int index);
    IndexInfo indexForHalfEdge(HalfEdge* halfedge);
    void expandBackward();
    void expandForward();
    void merge(MorphismPath* pathB);

    // Member variables
    std::vector<IndexInfo> indices;
    std::vector<Edge*>* edges;
    std::vector<HalfEdge*> halfedges;
    std::vector<bool> extendable;
    int id;
};

} // namespace ms 