#pragma once
#include <vector>
#include "../memory_counter.h"
#include "../geometry/vec2.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/half_edge.h"

class MorphismPath {
public:
    static int count;
    static MorphismPath* createPath(const vector<HalfEdge*>& halfEdges);

    MorphismPath(const vector<Edge*>& pathEdges);
    ~MorphismPath();

    void setHalfEdges(const vector<HalfEdge*>& halfEdges);
    bool isExtendable() const;
    Vertex* randomNextVertex();
    Vertex* rigidNextVertex();
    Edge* edgeFromIndex(int index);
    void expandBackward();
    void expandForward();
    void merge(MorphismPath* pathB);

    // Member variables
    vector<Edge*> pathEdges;
    vector<HalfEdge*> halfEdges;
    vector<bool> extendable;
    int id;
};

