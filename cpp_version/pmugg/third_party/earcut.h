#pragma once
#include <vector>
#include <memory>

namespace ms {
namespace earcut {

// Node structure for doubly-linked list
struct Node {
    Node(int i, float x, float y)
        : i(i), x(x), y(y), prev(nullptr), next(nullptr)
        , z(0), prevZ(nullptr), nextZ(nullptr), steiner(false) {}

    int i;
    float x;
    float y;
    Node* prev;
    Node* next;
    float z;
    Node* prevZ;
    Node* nextZ;
    bool steiner;
};

// Main triangulation function
std::vector<int> earcut(const std::vector<float>& data,
                       const std::vector<int>& holeIndices = {},
                       int dim = 2);

// Helper functions
std::unique_ptr<Node> linkedList(const std::vector<float>& data,
                               int start, int end, int dim, bool clockwise);
std::unique_ptr<Node> filterPoints(Node* start, Node* end = nullptr);
bool isEar(Node* ear);
bool isEarHashed(Node* ear, float minX, float minY, float invSize);
std::unique_ptr<Node> cureLocalIntersections(Node* start,
                                           std::vector<int>& triangles,
                                           int dim);
void splitEarcut(Node* start, std::vector<int>& triangles,
                 int dim, float minX, float minY, float invSize);
std::unique_ptr<Node> eliminateHoles(const std::vector<float>& data,
                                   const std::vector<int>& holeIndices,
                                   Node* outerNode, int dim);
Node* eliminateHole(Node* hole, Node* outerNode);
Node* findHoleBridge(Node* hole, Node* outerNode);
void indexCurve(Node* start, float minX, float minY, float invSize);
std::unique_ptr<Node> sortLinked(Node* list);
bool intersects(Node* p1, Node* q1, Node* p2, Node* q2);
bool locallyInside(Node* a, Node* b);
bool middleInside(Node* a, Node* b);
bool pointInTriangle(float ax, float ay, float bx, float by,
                    float cx, float cy, float px, float py);
bool isValidDiagonal(Node* a, Node* b);
float area(Node* p, Node* q, Node* r);
float signedArea(const std::vector<float>& data,
                int start, int end, int dim);
Node* getLeftmost(Node* start);
Node* splitPolygon(Node* a, Node* b);
int zOrder(float minX, float minY, float invSize, float x, float y);

} // namespace earcut
} // namespace ms 