#pragma once
#include <vector>
#include "../memory_counter.h"
#include "../geometry/vec2.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/half_edge.h"

// Represents a path along a particular face covering part of it.
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

    // This is only needed for handling rigid edges. There may be ways to speed this
    // up if we know all the edges can be stretched to any length.
    vector<Edge*> pathEdges;

    // The half-edges at the beginning and end of the path.
    HalfEdge* halfEdges[2];
    // Whether or not the path can be extended at the beginning or end meaning that
    // that vertex can be freed.
    bool extendable[2];
    int id;
};

