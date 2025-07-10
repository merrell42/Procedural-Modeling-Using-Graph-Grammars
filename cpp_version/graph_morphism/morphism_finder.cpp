#include "pch.h"
#include <algorithm>
#include <limits>
#include "morphism_finder.h"
#include "morphism.h"
#include "morphism_info.h"
#include "morphism_state.h"
#include "../primitives/edge_type.h"
#include "../geometry/intersector.h"
#include "../settings.h"
#include "../util/util.h"

map<int, VertexType*> MorphismFinder::splicedVertexTypes;

MorphismFinder::MorphismFinder(Model* model, bool groundEnabled)
    : model(model)
    , nodesModified(false)
    , groundEnabled(groundEnabled) {}

void MorphismFinder::reset() {
    nodesModified = false;
}

Morphism* MorphismFinder::findMorphism(Graph* graphB) {
    auto verticesB = graphB->getVertices();
    if (verticesB.empty()) {
        return findStarterMorphism(graphB);
    }

    // Start with a vertex that is not spliced.
    int index1 = 0;
    while (verticesB[index1]->getType()->getSpliced()) {
        index1++;
    }

    bool isOnBoundary = verticesB[index1]->boundaryIndex() >= 0;
    auto* vertexType = verticesB[index1]->getType();
    auto vertexMap = model->getCurrent()->getVertexMap();
    int N = (int)vertexMap.size();
    if (N == 0) {
        return nullptr;
    }

    int attempts = min(N, vertexAttempts);
    int startIndex = Util::randomInt(N);

    // Create a list of vertex IDs to try.
    vector<int> vertexIds;
    vertexIds.reserve(attempts);
    auto it = next(vertexMap.begin(), startIndex);
    for (int i = 0; i < attempts; i++) {
        vertexIds.push_back(it->first);
        ++it;
        if (it == vertexMap.end()) {
            it = vertexMap.begin();
        }
    }

    // Try each vertex until we find a match or run out of attempts
    for (int i = 0; i < attempts; i++) {
        int vertexId = vertexIds[i];
        auto* vertexA = model->getCurrent()->getVertex(vertexId);

        // Check that either vertex types match or we are on the boundary.
        if (isOnBoundary || vertexA->getType() == vertexType) {
            nodesModified = false;
            auto* info = new MorphismInfo(model, graphB);
            auto* state = new MorphismState(info);
            auto* morphism = assignVertex(state, vertexA, index1);
                
            if (morphism) {
                // addOuterFaces(morphism, graphB);
                delete state;
                delete info;
                return morphism;
            } else if (nodesModified) {
                model->reject();
            }
            delete state->getMorphism();
            delete state;
            delete info;
        }
    }
    return nullptr;
}


Morphism* MorphismFinder::assignVertex(MorphismState* state, Vertex* vertexA, int indexB) {
    state->assignVertex(vertexA, indexB);
    return findContinue(state);
}

// TODO: Implement this.For a closed building popping out of the ground.The ground face is the outer face.
/* void MorphismFinder::addOuterFaces(Morphism* map, Graph* graphB) {
    map->outerFaces.clear();
    auto outerFaces = graphB->getOuterFaces();
    for (size_t fIndex = 0; fIndex < outerFaces.size(); fIndex++) {
        auto* outerFaceB = outerFaces[fIndex];
        auto* outerHalf = outerFaceB->outerComponent;
        int vIndex = graphB->getInterior()->vertexIndex(outerHalf->getVertex());
        map->outerFaces.push_back(map->vertexBtoA[vIndex]->getHalfEdges()[outerHalf->vertexIndex]->getFace());
    }
} */

// Pick a random face with the given faceType.
Face* MorphismFinder::findFace(FaceType* faceType) {
    // The "Prefer Ground" setting biases the face selection to prefer picking the ground plane.
    // We pick the ground plane with the given probability.
    if (groundEnabled) {
        auto faces = model->getCurrent()->getFaceMap();
        if (!faces.empty()) {
            // Get the current ground face. It is the first face in faceMap.
            Face* currentGroundFace = faces.begin()->second;
            if (faceType == currentGroundFace->getFaceType()) {
                auto preference = globalSettings["Prefer Ground"].get<double>();
                if (Util::randomUniform(0, 1) < preference) {
                    return currentGroundFace;
                }
            }
        }
    }

    // This could be made more efficient, but this doesn't take much of the computation time.
    auto facesA = model->getCurrent()->getFaceMap();
    vector<Face*> facesVec;
    for (const auto& [_, face] : facesA) {
        facesVec.push_back(face);
    }

    // Get a selection of faces with the given faceType. Weight each face by it's surface area.
    // Randomly pick a face using the weights.
    int N = (int)facesVec.size();
    int attempts = min(N, faceAttempts);
    int startIndex = Util::randomInt(N);
    vector<Face*> options;
    vector<double> weights;
    for (int i = 0; i < attempts; i++) {
        auto* faceA = facesVec[(startIndex + i) % N];
        if (faceA && faceA->getFaceType() == faceType && !faceA->isHole()) {
            weights.push_back(abs(faceA->signedArea()));
            options.push_back(faceA);
        }
    }

    if (!weights.empty()) {
        return options[Util::randomDistribution(weights)];
    }
    return nullptr;
}

Morphism* MorphismFinder::findStarterMorphism(Graph* graphB) {
    auto info = make_unique<MorphismInfo>(model, graphB);
    auto state = make_unique<MorphismState>(info.get());

    // If no boundary faces, we are done. The shape is being generated in empty space.
    auto faces = graphB->getFaces();
    if (faces.empty()) {
        return state->getMorphism();
    }

    auto faceMap = model->getCurrent()->getFaceMap();
    if (!groundEnabled || faceMap.empty()) {
        return state->getMorphism();
    }

    auto bFaces = graphB->getBFaces();
    if (bFaces.size() != 1) {
        cout << "Expect one boundary face." << endl;
        delete state->getMorphism();
        return nullptr;
    }

    // Find a face with the given type.
    Face* face = findFace(bFaces[0]->getType());
    if (face) {
        state->getMorphism()->faceBtoA = {face};

        // The web version supports cases where we have disconnected boundaries and
        // do a volume splice through castVolumeRaySeries.
        // This was always hacky and so I have omitted it here.
        return state->getMorphism();
    }
    delete state->getMorphism();
    return nullptr;
}

// Continue finding the morphism. We continue until all half-edges in the two queues
// have been processed. The normal queue takes precedence over the splice queue.
Morphism* MorphismFinder::findContinue(MorphismState* state) {
    if (!state->getQueue().empty()) {
        vector<HalfEdgeData>& queue = state->getQueue();
        HalfEdgeData halfEdgeData = queue.front();
        queue.erase(queue.begin());
        return matchHalfEdge(halfEdgeData, state);
    } else if (!state->getSpliceQueue().empty()) {
        vector<HalfEdgeData>& queue = state->getSpliceQueue();
        HalfEdgeData halfEdgeData = queue.front();
        queue.erase(queue.begin());
        return matchSplicedHalfEdge(halfEdgeData, state);
    }
    return state->getMorphism();
}

// Check if edgeA has a half-edge on a face with holes.
bool neighboringHoles(Edge* edgeA, GraphEdge* edgeB) {
    const auto& halfEdgesA = edgeA->getHalfEdges();
    const auto& halfEdgesB = edgeB->getHalfEdges();

    for (int i = 0; i < halfEdgesA.size(); i++) {
        auto halfEdge = halfEdgesA[i];
        Face* face = halfEdge->getFace();
        bool hasHoles = (face->getGroup()->getFaces().size() > 1) && !face->isHole();
        if (hasHoles && halfEdgesB[i][0]->isLoopy()) {
            return true;
        }
    }
    return false;
}

// Attempt to find a match for a half-edge at a particular vertex.
// Try all the half-edges of that vertex.
Morphism* MorphismFinder::matchHalfEdge(
    const HalfEdgeData& halfEdgeData,
    MorphismState* state
) {
    // Find the desired edge type: typeB.
    GraphHalfEdge* halfB = halfEdgeData.halfB;
    GraphEdge* edgeB = halfB->getEdge();
    if (!edgeB) {
        return findContinue(state);
    }
    EdgeType* typeB = edgeB->getType();

    // Check every half-edge of vertexA for a match with the same edge-type and direction.
    Vertex* vertexA = halfEdgeData.vertexA;
    auto halfsA = vertexA->getHalfEdges();
    HalfEdge* matchHalfA = nullptr;
    for (auto* halfA : halfsA) {
        if (halfA->getEdgeType() == typeB &&
            halfA->getIsAtStart() == halfB->getForward()) {
            matchHalfA = halfA;
            break;
        }
    }
    if (!matchHalfA) {
        return nullptr;
    }

    // Do not use this half-edge if it is attached to a face with interior vertex and
    // it is not an outer face.
    auto faceB = halfB->getFace();
    auto bFaces = faceB->getGraph()->getBFaces();
    bool isOuter = !faceB->isLoopy() || contains(bFaces, faceB);
    if (!isOuter && neighboringHoles(matchHalfA->getEdge(), edgeB)) {
        return nullptr;
    }
    return assignHalfEdge(matchHalfA, halfB, state);
}

Morphism* MorphismFinder::matchSplicedHalfEdge(
    const HalfEdgeData& halfEdgeData,
    MorphismState* state
) {
    GraphHalfEdge* halfB = halfEdgeData.halfB;

    // Check if that we haven't already added the spliced half-edge to the morphism.
    // Follow the chain of spliced half-edges to the end.
    GraphHalfEdge* endB = halfB;
    while (endB->isSpliced()) {
        endB = endB->getNext();
    }
    // Check if the end is already in the morphism.
    int vIndexB = indexOf(state->getInfo()->verticesB, endB->getVertex());
    if (state->getMorphism()->vertexBtoA[vIndexB]) {
        return findContinue(state);
    }

    // Find a half-edge on a face with faceTypeB.
    HalfEdge* matchHalfA = nullptr;
    Vertex* vertexA = halfEdgeData.vertexA;
    FaceType* faceTypeB = halfB->getEdge()->getType()->getFaceData()[0].type;
    for (auto* halfA : vertexA->getHalfEdges()) {
        if (halfA->getFace()->getFaceType() == faceTypeB) {
            matchHalfA = halfA;
            break;
        }
    }
    // Return failure if no match is found.
    if (!matchHalfA) {
        return nullptr;
    }

    // Cast a series of rays representing the spliced half-edges and try to intersect the right edges.
    int attempts = 0;
    int maxDim = faceTypeB->getMaxDim();
    const Vec3 startPos = vertexA->getPosition();
    FaceGroup* groupA = matchHalfA->getFace()->getGroup();
    HalfEdge* intersectedHalf = nullptr;
    while (attempts < spliceRayAttempts && !intersectedHalf) {
        auto faceMap = model->getCurrent()->getFaceMap();
        intersectedHalf = castSeriesOfRays(halfB, startPos, groupA, faceMap, maxDim);
        attempts++;
    }
    // Return failure if no intersection found.
    if (!intersectedHalf) {
        return nullptr;
    }

    // The splice will be inserted between these two vertices.
    Vertex* vertexA0 = intersectedHalf->getVertex();
    Vertex* vertexA1 = intersectedHalf->next()->getVertex();

    int vIndex0 = indexOf(state->getMorphism()->getVertexBtoA(), vertexA0);
    int vIndex1 = indexOf(state->getMorphism()->getVertexBtoA(), vertexA1);
    int eIndex = indexOf(state->getMorphism()->getEdgeBtoA(), intersectedHalf->getEdge());
    Graph* graphB = state->getInfo()->graphB;

    if (vIndex0 < 0 && vIndex1 < 0 && eIndex < 0) {
        // If the vertices and edges have not already been matched in the morphism, split the edge once.
        auto result = intersectedHalf->getEdge()->fullSplit(Util::randomUniform(0, 1));
        nodesModified = true;
        return assignVertex(state, result.second, vIndexB);
    } else if (vIndex0 >= 0 && vIndex1 >= 0 && eIndex >= 0) {
        // If the vertices and edge have already been matched in the morphism, split the edge three times.
        bool isAtStart0 = intersectedHalf->getIsAtStart();
        GraphVertex* vertexB0 = graphB->getVertices()[vIndex0];
        GraphVertex* vertexB1 = graphB->getVertices()[vIndex1];

        bool isOnBoundary0 = (vertexB0->boundaryIndex() >= 0);
        bool isOnBoundary1 = (vertexB1->boundaryIndex() >= 0);

        if (!(isOnBoundary0 ^ isOnBoundary1)) {
            cout << "Expected one of the vertices to be on the boundary." << endl;
        }

        bool atStart = isOnBoundary0 ? isAtStart0 : !isAtStart0;
        nodesModified = true;

        // Perform triple split.
        vector<Edge*> splitEdges;
        vector<Vertex*> splitVertices;
        Edge* edgeToSplit = intersectedHalf->getEdge();

        for (int i = 0; i < 3; i++) {
            auto result = edgeToSplit->fullSplit(1.0 / (4 - i));
            splitVertices.push_back(result.second);
            splitEdges.push_back(result.first.edges[0]);
            edgeToSplit = result.first.edges[1];

            if (i == 2) {
                splitEdges.push_back(edgeToSplit);
            }
        }

        int vIndexBoundary = isOnBoundary0 ? vIndex0 : vIndex1;
        if (atStart) {
            state->getMorphism()->vertexBtoA[vIndexBoundary] = splitVertices[2];
            state->getMorphism()->edgeBtoA[eIndex] = splitEdges[3];
            return assignVertex(state, splitVertices[0], vIndexB);
        } else {
            state->getMorphism()->vertexBtoA[vIndexBoundary] = splitVertices[0];
            state->getMorphism()->edgeBtoA[eIndex] = splitEdges[0];
            return assignVertex(state, splitVertices[2], vIndexB);
        }
    } else if (vIndex0 < 0 || vIndex1 < 0 || eIndex < 0) {
        cout << "Unsure what to do about a partial match" << endl;
    }

    return nullptr;
}

// Assign half-edge B to half-edge A.
Morphism* MorphismFinder::assignHalfEdge(HalfEdge* halfA, GraphHalfEdge* halfB, MorphismState* state) {
    auto info = state->getInfo();
    auto morphism = state->getMorphism();

    GraphVertex* vertexB = halfB->getNext()->getVertex();
    int vIndexB = indexOf(info->verticesB, vertexB);

    // Split halfA if it's a spliced vertex.
    bool isOnBoundary = vertexB->boundaryIndex() >= 0;
    auto vertexType = vertexB->getType();
    if (!isOnBoundary && vertexType->getSpliced() && !morphism->vertexBtoA[vIndexB]) {
        halfA->getEdge()->fullSplit(random());
        nodesModified = true;
    }

    // Assign the edge mapping from B to A.
    Edge* edgeA = halfA->getEdge();
    GraphEdge* edgeB = halfB->getEdge();
    int eIndexB = indexOf(info->edgesB, edgeB);
    morphism->edgeBtoA[eIndexB] = edgeA;

    // See if we have already found a match for vertex A.
    Vertex* vertexA = halfA->next()->getVertex();
    int vIndexA = indexOf(morphism->vertexBtoA, vertexA);
    if (vIndexA < morphism->vertexBtoA.size()) {
        if (isOnBoundary || vIndexA == vIndexB) {
            // We've already matched vertex A to the end of halfB.
            return findContinue(state);
        } else {
            // We've match vertex A to a different vertex. The graphs do not match.
            return nullptr;
        }
    }

    // Assign vertex A if it has the right type or it's on the boundary or it's a splice vertex.
    if (isOnBoundary || vertexA->getType() == vertexType || vertexType->getSpliced()) {
        return assignVertex(state, vertexA, vIndexB);
    } else {
        return nullptr;
    }
}

// Find the first intersection between an edge and a face.
// The first intersection is closest to the start position of the edge.
void MorphismFinder::findFirstIntersection(Face* faceA, const Vec3& p0, const Vec3& p1, const Vec2& dir2, IntersectResult& firstIntersection, int maxDim) {
    vector<Vec3> fPositions = faceA->getPositions();
    vector<IntersectionData> intersections = Intersector::edgeFaceIntersect(p0, p1, fPositions, maxDim);
    
    for (const IntersectionData& intersection : intersections) {
        double distance = dir2.dot(intersection.pos);
        if (distance < firstIntersection.distance) {
            // Delete previous data if it exists.
            if (firstIntersection.data != nullptr) {
                delete firstIntersection.data;
            }
            firstIntersection.distance = distance;
            firstIntersection.face = faceA;
            firstIntersection.data = new IntersectionData(intersection);
        }
    }
}

// Cast a ray starting from p0 in direction dir. Find the first intersection.
// TODO: Use BSP tree.
IntersectResult MorphismFinder::castRay(const Vec3& p0, const Vec3& dir, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim) {
    Vec3 p1 = dir;
    p1.scale(Intersector::FAR_DISTANCE);
    p1.add(p0);
    Vec2 dir2 = dir.dropDim(maxDim);

    IntersectResult firstIntersection;
    firstIntersection.distance = numeric_limits<double>::infinity();
    firstIntersection.face = nullptr;
    firstIntersection.data = nullptr;

    // Check all faces in the group meaning the outer face and the holes.
    for (Face* faceA : groupA->getFaces()) {
        findFirstIntersection(faceA, p0, p1, dir2, firstIntersection, maxDim);
    }
    // Closed 3D faces should have an intersection here.
    if (firstIntersection.face) {
        return firstIntersection;
    }

    // For 2D intersections, search random faces from the map.
    int N = (int)faceMap.size();
    int attempts = min(N, faceAttempts);
    int startIndex = Util::randomInt(N);
    
    auto it = next(faceMap.begin(), startIndex % N);
    for (int i = 0; i < attempts; i++) {
        findFirstIntersection(it->second, p0, p1, dir2, firstIntersection, maxDim);
        
        // Move to next face, wrapping around to beginning if needed
        ++it;
        if (it == faceMap.end()) {
            it = faceMap.begin();
        }
    }
    return firstIntersection;
}

// Cast a series of rays for splicing. Start at halfB and continue until the
// until we've completed the series of spliced half-edges.
HalfEdge* MorphismFinder::castSeriesOfRays(GraphHalfEdge* halfB, const Vec3& startPos, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim) {
    Vec3 p0 = startPos;
    GraphHalfEdge* nextB = halfB->getNext();
    
    while (true) {
        const Vec3 dir = halfB->getDir();
        IntersectResult firstIntersection = castRay(p0, dir, groupA, faceMap, maxDim);

        if (!firstIntersection.data) {
            return nullptr;
        }

        // If nextB is spliced, move out a random distance that is before the first intersection.
        if (nextB->isSpliced()) {
            Vec2 dir2 = dir.dropDim(maxDim);
            double scale = dir.length() / dir2.length();
            double nearestDist = scale * firstIntersection.distance;
            double maxDistance = max(nearestDist, (double)maxRayDistance);
            double d = Util::randomUniform(0, maxDistance);

            // TODO: This looks wrong. Check it.
            p0 = dir;
            p0.scale(d);
            p0.add(startPos);
        } else {
            // Check that the first intersection is an edge with the correct type.
            HalfEdge* nextA = firstIntersection.face->getHalfEdges()[firstIntersection.data->index];
            EdgeType* edgeTypeA = nextA->getEdgeType();
            EdgeType* edgeTypeB = nextB->getEdge()->getType();

            delete firstIntersection.data;
            if (edgeTypeA != edgeTypeB) {
                return nullptr;
            } else {
                return nextA;
            }
        }

        delete firstIntersection.data;
        halfB = nextB;
        nextB = nextB->getNext();
    }
}

