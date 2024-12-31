#include "earcut.h"
#include <algorithm>
#include <cmath>

namespace ms {
namespace earcut {

std::vector<int> earcut(const std::vector<float>& data,
                       const std::vector<int>& holeIndices,
                       int dim) {
    dim = dim || 2;

    std::vector<int> triangles;
    bool hasHoles = !holeIndices.empty();
    int outerLen = hasHoles ? holeIndices[0] * dim : data.size();

    auto outerNode = linkedList(data, 0, outerLen, dim, true);
    if (!outerNode || outerNode->next == outerNode->prev) {
        return triangles;
    }

    float minX = 0;
    float minY = 0;
    float maxX = 0;
    float maxY = 0;
    float invSize = 0;

    if (hasHoles) {
        outerNode = eliminateHoles(data, holeIndices, outerNode.get(), dim);
    }

    // If the shape is not too simple, we'll use z-order curve hash later
    if (data.size() > 80 * dim) {
        minX = maxX = data[0];
        minY = maxY = data[1];

        for (int i = dim; i < outerLen; i += dim) {
            float x = data[i];
            float y = data[i + 1];
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }

        // minX, minY and invSize are later used to transform coords into integers
        invSize = std::max(maxX - minX, maxY - minY);
        invSize = invSize != 0 ? 1 / invSize : 0;
    }

    earcutLinked(outerNode.get(), triangles, dim, minX, minY, invSize);

    return triangles;
}

void earcutLinked(Node* ear, std::vector<int>& triangles,
                  int dim, float minX, float minY, float invSize,
                  int pass = 0) {
    if (!ear) return;

    // Interlink polygon nodes in z-order
    if (!pass && invSize) indexCurve(ear, minX, minY, invSize);

    Node* stop = ear;
    Node* prev;
    Node* next;

    // Iterate through ears, slicing them one by one
    while (ear->prev != ear->next) {
        prev = ear->prev;
        next = ear->next;

        if (invSize ? isEarHashed(ear, minX, minY, invSize) : isEar(ear)) {
            // Cut off the triangle
            triangles.push_back(prev->i / dim);
            triangles.push_back(ear->i / dim);
            triangles.push_back(next->i / dim);

            // Remove ear node
            next->prev = prev;
            prev->next = next;

            if (ear->prevZ) ear->prevZ->nextZ = ear->nextZ;
            if (ear->nextZ) ear->nextZ->prevZ = ear->prevZ;

            // Skipping the next vertex leads to less sliver triangles
            ear = next->next;
            stop = next->next;

            continue;
        }

        ear = next;

        // If we looped through the whole remaining polygon and can't find any more ears
        if (ear == stop) {
            // Try filtering points and slicing again
            if (!pass) {
                earcutLinked(filterPoints(ear).get(), triangles, dim, minX, minY, invSize, 1);
            } else if (pass == 1) {
                // If this didn't work, try curing all small self-intersections locally
                ear = cureLocalIntersections(ear, triangles, dim).get();
                earcutLinked(ear, triangles, dim, minX, minY, invSize, 2);
            } else if (pass == 2) {
                // As a last resort, try splitting the remaining polygon into two
                splitEarcut(ear, triangles, dim, minX, minY, invSize);
            }
            break;
        }
    }
}

// Check whether a polygon node forms a valid ear with adjacent nodes
bool isEar(Node* ear) {
    Node* a = ear->prev;
    Node* b = ear;
    Node* c = ear->next;

    if (area(a, b, c) >= 0) return false; // Reflex, can't be an ear

    // Now make sure we don't have other points inside the potential ear
    Node* p = ear->next->next;

    while (p != ear->prev) {
        if (pointInTriangle(a->x, a->y, b->x, b->y, c->x, c->y, p->x, p->y) &&
            area(p->prev, p, p->next) >= 0) return false;
        p = p->next;
    }

    return true;
}

// Check whether a polygon node forms a valid ear with adjacent nodes using z-order curve value
bool isEarHashed(Node* ear, float minX, float minY, float invSize) {
    Node* a = ear->prev;
    Node* b = ear;
    Node* c = ear->next;

    if (area(a, b, c) >= 0) return false; // Reflex, can't be an ear

    // Triangle bbox; min & max are calculated like this for speed
    float minTX = std::min(a->x, std::min(b->x, c->x));
    float minTY = std::min(a->y, std::min(b->y, c->y));
    float maxTX = std::max(a->x, std::max(b->x, c->x));
    float maxTY = std::max(a->y, std::max(b->y, c->y));

    // Z-order range for the current triangle bbox
    int minZ = zOrder(minX, minY, invSize, minTX, minTY);
    int maxZ = zOrder(minX, minY, invSize, maxTX, maxTY);

    // First look for points inside the triangle in increasing z-order
    Node* p = ear->nextZ;

    while (p && p->z <= maxZ) {
        if (p != ear->prev && p != ear->next &&
            pointInTriangle(a->x, a->y, b->x, b->y, c->x, c->y, p->x, p->y) &&
            area(p->prev, p, p->next) >= 0) return false;
        p = p->nextZ;
    }

    // Then look for points in decreasing z-order
    p = ear->prevZ;

    while (p && p->z >= minZ) {
        if (p != ear->prev && p != ear->next &&
            pointInTriangle(a->x, a->y, b->x, b->y, c->x, c->y, p->x, p->y) &&
            area(p->prev, p, p->next) >= 0) return false;
        p = p->prevZ;
    }

    return true;
}

// Go through all polygon nodes and cure small local self-intersections
std::unique_ptr<Node> cureLocalIntersections(Node* start,
                                           std::vector<int>& triangles,
                                           int dim) {
    Node* p = start;
    do {
        Node* a = p->prev;
        Node* b = p->next->next;

        // A self-intersection where edge (v[i-1],v[i]) intersects (v[i+1],v[i+2])
        if (!equals(a, b) && intersects(a, p, p->next, b) &&
            locallyInside(a, b) && locallyInside(b, a)) {
            triangles.push_back(a->i / dim);
            triangles.push_back(p->i / dim);
            triangles.push_back(b->i / dim);

            // Remove two nodes involved
            p->next->next = b->next;
            b->next->prev = p->next;

            p = start = b;
        }
        p = p->next;
    } while (p != start);

    return filterPoints(p);
}

// Try splitting polygon into two and triangulate them independently
void splitEarcut(Node* start, std::vector<int>& triangles,
                 int dim, float minX, float minY, float invSize) {
    // Look for a valid diagonal that divides the polygon into two
    Node* a = start;
    do {
        Node* b = a->next->next;
        while (b != a->prev) {
            if (a->i != b->i && isValidDiagonal(a, b)) {
                // Split the polygon in two by the diagonal
                Node* c = splitPolygon(a, b);

                // Filter colinear points around the cuts
                a = filterPoints(a, a->next).get();
                c = filterPoints(c, c->next).get();

                // Run earcut on each half
                earcutLinked(a, triangles, dim, minX, minY, invSize);
                earcutLinked(c, triangles, dim, minX, minY, invSize);
                return;
            }
            b = b->next;
        }
        a = a->next;
    } while (a != start);
}

// Link every hole into the outer loop, producing a single-ring polygon without holes
std::unique_ptr<Node> eliminateHoles(const std::vector<float>& data,
                                   const std::vector<int>& holeIndices,
                                   Node* outerNode, int dim) {
    std::vector<Node*> queue;

    for (size_t i = 0; i < holeIndices.size(); i++) {
        int start = holeIndices[i] * dim;
        int end = i < holeIndices.size() - 1 ? holeIndices[i + 1] * dim : data.size();
        auto list = linkedList(data, start, end, dim, false);
        if (list->next == list->prev) list->steiner = true;
        queue.push_back(getLeftmost(list.get()));
    }

    std::sort(queue.begin(), queue.end(),
             [](const Node* a, const Node* b) { return a->x < b->x; });

    // Process holes from left to right
    for (auto* hole : queue) {
        eliminateHole(hole, outerNode);
        outerNode = filterPoints(outerNode, outerNode->next).get();
    }

    return std::unique_ptr<Node>(outerNode);
}

// Find a bridge between vertices that connects hole with an outer ring and link it
Node* eliminateHole(Node* hole, Node* outerNode) {
    Node* bridge = findHoleBridge(hole, outerNode);
    if (!bridge) {
        return outerNode;
    }

    Node* bridgeReverse = splitPolygon(bridge, hole);

    // Filter out colinear points around cuts
    auto filteredBridge = filterPoints(bridge, bridge->next);
    filterPoints(bridgeReverse, bridgeReverse->next);

    // Check if input node was removed by the filtering
    return outerNode == bridge ? filteredBridge.get() : outerNode;
}

// David Eberly's algorithm for finding a bridge between hole and outer polygon
Node* findHoleBridge(Node* hole, Node* outerNode) {
    Node* p = outerNode;
    float hx = hole->x;
    float hy = hole->y;
    float qx = -INFINITY;
    Node* m = nullptr;

    // Find a segment intersected by a ray from the hole's leftmost point to the left;
    // segment's endpoint with lesser x will be potential connection point
    do {
        if (hy <= p->y && hy >= p->next->y && p->next->y != p->y) {
            float x = p->x + (hy - p->y) * (p->next->x - p->x) / (p->next->y - p->y);
            if (x <= hx && x > qx) {
                qx = x;
                if (x == hx) {
                    if (hy == p->y) return p;
                    if (hy == p->next->y) return p->next;
                }
                m = p->x < p->next->x ? p : p->next;
            }
        }
        p = p->next;
    } while (p != outerNode);

    if (!m) return nullptr;

    if (hx == qx) return m->prev; // hole touches outer segment; pick leftmost endpoint

    // Look for points inside the triangle of hole point, segment intersection and endpoint;
    // if there are no points found, we have a valid connection;
    // otherwise choose the point of the minimum angle with the ray as connection point

    Node* stop = m;
    float mx = m->x;
    float my = m->y;
    float tanMin = INFINITY;
    float tan;

    p = m->next;

    while (p != stop) {
        if (hx >= p->x && p->x >= mx && hx != p->x &&
            pointInTriangle(hy < my ? hx : qx, hy, mx, my, hy < my ? qx : hx, hy, p->x, p->y)) {

            tan = std::abs(hy - p->y) / (hx - p->x); // tangential

            if ((tan < tanMin || (tan == tanMin && p->x > m->x)) &&
                locallyInside(p, hole)) {
                m = p;
                tanMin = tan;
            }
        }

        p = p->next;
    }

    return m;
}

// Interlink polygon nodes in z-order
void indexCurve(Node* start, float minX, float minY, float invSize) {
    Node* p = start;
    do {
        if (p->z == 0) p->z = zOrder(minX, minY, invSize, p->x, p->y);
        p->prevZ = p->prev;
        p->nextZ = p->next;
        p = p->next;
    } while (p != start);

    p->prevZ->nextZ = nullptr;
    p->prevZ = nullptr;

    sortLinked(p);
}

// Simon Tatham's linked list merge sort algorithm
// http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
std::unique_ptr<Node> sortLinked(Node* list) {
    int i, p, q, e, numMerges, pSize, qSize;
    int inSize = 1;

    do {
        p = list;
        list = nullptr;
        Node* tail = nullptr;
        numMerges = 0;

        while (p) {
            numMerges++;
            q = p;
            pSize = 0;
            for (i = 0; i < inSize; i++) {
                pSize++;
                q = q->nextZ;
                if (!q) break;
            }

            qSize = inSize;

            while (pSize > 0 || (qSize > 0 && q)) {
                Node* e;

                if (pSize == 0) {
                    e = q;
                    q = q->nextZ;
                    qSize--;
                } else if (qSize == 0 || !q) {
                    e = p;
                    p = p->nextZ;
                    pSize--;
                } else if (p->z <= q->z) {
                    e = p;
                    p = p->nextZ;
                    pSize--;
                } else {
                    e = q;
                    q = q->nextZ;
                    qSize--;
                }

                if (tail) tail->nextZ = e;
                else list = e;

                e->prevZ = tail;
                tail = e;
            }

            p = q;
        }

        tail->nextZ = nullptr;
        inSize *= 2;

    } while (numMerges > 1);

    return std::unique_ptr<Node>(list);
}

// Create a circular doubly linked list from polygon points in the specified winding order
std::unique_ptr<Node> linkedList(const std::vector<float>& data,
                               int start, int end, int dim, bool clockwise) {
    Node* lastNode = nullptr;
    
    if (clockwise == (signedArea(data, start, end, dim) > 0)) {
        for (int i = start; i < end; i += dim) {
            lastNode = insertNode(i, data[i], data[i + 1], lastNode);
        }
    } else {
        for (int i = end - dim; i >= start; i -= dim) {
            lastNode = insertNode(i, data[i], data[i + 1], lastNode);
        }
    }

    if (lastNode && equals(lastNode, lastNode->next)) {
        removeNode(lastNode);
        lastNode = lastNode->next;
    }

    return std::unique_ptr<Node>(lastNode);
}

// Eliminate colinear or duplicate points
std::unique_ptr<Node> filterPoints(Node* start, Node* end) {
    if (!start) return nullptr;
    if (!end) end = start;

    Node* p = start;
    bool again;

    do {
        again = false;

        if (!p->steiner && (equals(p, p->next) ||
            area(p->prev, p, p->next) == 0)) {
            removeNode(p);
            p = end = p->prev;

            if (p == p->next) {
                p = nullptr;
                break;
            }
            again = true;
        } else {
            p = p->next;
        }
    } while (again || p != end);

    return std::unique_ptr<Node>(end);
}

// Insert a new node into a doubly linked list
Node* insertNode(int i, float x, float y, Node* last) {
    auto* p = new Node(i, x, y);

    if (!last) {
        p->prev = p;
        p->next = p;
    } else {
        p->next = last->next;
        p->prev = last;
        last->next->prev = p;
        last->next = p;
    }
    return p;
}

void removeNode(Node* p) {
    p->next->prev = p->prev;
    p->prev->next = p->next;
}

// Check if two points are equal
bool equals(const Node* p1, const Node* p2) {
    return p1->x == p2->x && p1->y == p2->y;
}

// Calculate polygon area
float area(Node* p, Node* q, Node* r) {
    return (q->y - p->y) * (r->x - q->x) - (q->x - p->x) * (r->y - q->y);
}

float signedArea(const std::vector<float>& data, int start, int end, int dim) {
    float sum = 0;
    for (int i = start, j = end - dim; i < end; i += dim) {
        sum += (data[i] - data[j]) * (data[i + 1] + data[j + 1]);
        j = i;
    }
    return sum;
}

// Check if a point lies within a convex triangle
bool pointInTriangle(float ax, float ay, float bx, float by, float cx, float cy,
                    float px, float py) {
    return (cx - px) * (ay - py) - (ax - px) * (cy - py) >= 0 &&
           (ax - px) * (by - py) - (bx - px) * (ay - py) >= 0 &&
           (bx - px) * (cy - py) - (cx - px) * (by - py) >= 0;
}

// Check if a diagonal between two polygon nodes is valid (lies in polygon interior)
bool isValidDiagonal(Node* a, Node* b) {
    return a->next->i != b->i && a->prev->i != b->i && !intersectsPolygon(a, b) &&
           locallyInside(a, b) && locallyInside(b, a) && middleInside(a, b);
}

// Check if two segments intersect
bool intersects(Node* p1, Node* q1, Node* p2, Node* q2) {
    if ((equals(p1, q1) && equals(p2, q2)) ||
        (equals(p1, q2) && equals(p2, q1))) return true;
    return area(p1, q1, p2) > 0 != area(p1, q1, q2) > 0 &&
           area(p2, q2, p1) > 0 != area(p2, q2, q1) > 0;
}

// Check if a polygon diagonal intersects any polygon segments
bool intersectsPolygon(Node* a, Node* b) {
    Node* p = a;
    do {
        if (p->i != a->i && p->next->i != a->i && p->i != b->i && p->next->i != b->i &&
            intersects(p, p->next, a, b)) return true;
        p = p->next;
    } while (p != a);
    return false;
}

// Check if a polygon diagonal is locally inside the polygon
bool locallyInside(Node* a, Node* b) {
    return area(a->prev, a, a->next) < 0 ?
        area(a, b, a->next) >= 0 && area(a, a->prev, b) >= 0 :
        area(a, b, a->prev) < 0 || area(a, a->next, b) < 0;
}

// Check if the middle point of a polygon diagonal is inside the polygon
bool middleInside(Node* a, Node* b) {
    Node* p = a;
    bool inside = false;
    float px = (a->x + b->x) / 2;
    float py = (a->y + b->y) / 2;
    do {
        if (((p->y > py) != (p->next->y > py)) &&
            (px < (p->next->x - p->x) * (py - p->y) / (p->next->y - p->y) + p->x))
            inside = !inside;
        p = p->next;
    } while (p != a);
    return inside;
}

// Find the leftmost node of a polygon ring
Node* getLeftmost(Node* start) {
    Node* p = start;
    Node* leftmost = start;
    do {
        if (p->x < leftmost->x || (p->x == leftmost->x && p->y < leftmost->y))
            leftmost = p;
        p = p->next;
    } while (p != start);
    return leftmost;
}

// Split polygon into two parts by the given diagonal
Node* splitPolygon(Node* a, Node* b) {
    Node* a2 = new Node(a->i, a->x, a->y);
    Node* b2 = new Node(b->i, b->x, b->y);
    Node* an = a->next;
    Node* bp = b->prev;

    a->next = b;
    b->prev = a;

    a2->next = an;
    an->prev = a2;

    b2->next = a2;
    a2->prev = b2;

    bp->next = b2;
    b2->prev = bp;

    return b2;
}

// Calculate z-order of a point given coords and inverse of the longer side of data bbox
int zOrder(float minX, float minY, float invSize, float x, float y) {
    // coords are transformed into non-negative 15-bit integer range
    int x2 = static_cast<int>(32767 * (x - minX) * invSize);
    int y2 = static_cast<int>(32767 * (y - minY) * invSize);

    x2 = (x2 | (x2 << 8)) & 0x00FF00FF;
    x2 = (x2 | (x2 << 4)) & 0x0F0F0F0F;
    x2 = (x2 | (x2 << 2)) & 0x33333333;
    x2 = (x2 | (x2 << 1)) & 0x55555555;

    y2 = (y2 | (y2 << 8)) & 0x00FF00FF;
    y2 = (y2 | (y2 << 4)) & 0x0F0F0F0F;
    y2 = (y2 | (y2 << 2)) & 0x33333333;
    y2 = (y2 | (y2 << 1)) & 0x55555555;

    return x2 | (y2 << 1);
}

} // namespace earcut
} // namespace ms 