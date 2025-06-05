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
    static MorphismPath* createPath(const vector<HalfEdge*>& halfedges,
                                   const vector<Edge*>& edges,
                                   vector<Edge*>* lines);

    MorphismPath(const vector<IndexInfo>& indices, vector<Edge*>* edges);

    void setHalfEdges(const vector<HalfEdge*>& halfedges);
    int extendableness() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Edge* edgeFromIndex(int index);
    IndexInfo indexForHalfEdge(HalfEdge* halfedge);
    void expandBackward();
    void expandForward();
    void merge(MorphismPath* pathB);

    // Member variables
    vector<IndexInfo> indices;
    vector<Edge*>* edges;
    vector<HalfEdge*> halfedges;
    vector<bool> extendable;
    int id;
};

} // namespace ms 