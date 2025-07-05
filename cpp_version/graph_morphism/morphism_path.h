#pragma once
#include <vector>
#include "../memory_counter.h"
#include "../geometry/vec2.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/half_edge.h"

class MorphismPath {
public:
    struct IndexInfo {
        int index;
        bool isForward;
    };

    static int count;
    static MorphismPath* createPath(const vector<HalfEdge*>& halfEdges,
                                   const vector<Edge*>& edges,
                                   vector<Edge*>* lines);

    MorphismPath(const vector<IndexInfo>& indices, vector<Edge*>* edges);
    ~MorphismPath();

    void setHalfEdges(const vector<HalfEdge*>& halfEdges);
    bool isExtendable() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Edge* edgeFromIndex(int index);
    IndexInfo indexForHalfEdge(HalfEdge* halfEdge);
    void expandBackward();
    void expandForward();
    void merge(MorphismPath* pathB);

    // Member variables
    vector<IndexInfo> indices;
    vector<Edge*>* edges;
    vector<HalfEdge*> halfEdges;
    vector<bool> extendable;
    int id;
};

