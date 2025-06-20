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

unordered_map<int, VertexType*> MorphismFinder::splicedVertexTypes;

MorphismFinder::MorphismFinder(Model* model, bool groundEnabled)
    : model(model)
    , nodesModified(false)
    , groundFace(nullptr)
    , groundEnabled(groundEnabled) {}

void MorphismFinder::reset() {
    nodesModified = false;
    groundFace = nullptr;
}

Morphism* MorphismFinder::findMorphism(Graph* graphB) {
    if (!groundFace && groundEnabled) {
        auto faces = model->getCurrent()->getFaceMap();
        if (!faces.empty()) {
            groundFace = faces.begin()->second;
        }
    }

    auto verticesB = graphB->getVertices();
    if (verticesB.empty()) {
        return findStarterMap(graphB);
    }

    // Start with a vertex that is not spliced
    int index1 = 0;
    while (verticesB[index1]->getType()->getSpliced()) {
        index1++;
    }

    bool isConnector = verticesB[index1]->boundaryIndex() >= 0;
    auto* vertexType = verticesB[index1]->getType();
    auto vertexMap = model->getCurrent()->getVertexMap();
    int N = (int)vertexMap.size();
    if (N == 0) {
        return nullptr;
    }

    int attempts = min(N, vertexAttempts);
    int startIndex = Util::randomInt(N);

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

    for (int i = 0; i < attempts; i++) {
        int vertexId = vertexIds[i];
        auto* vertexA = model->getCurrent()->getVertex(vertexId);
        
        if (isConnector || vertexA->getType() == vertexType) {
            nodesModified = false;
            auto* info = new MorphismInfo(model, graphB);
            auto* state = new MorphismState(info);
            auto* morphism = assignVertex(state, vertexA, index1);
                
            if (morphism) {
                addOuterFaces(morphism, graphB);
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

void MorphismFinder::addOuterFaces(Morphism* map, Graph* graphB) {
    // TODO: Implement this. For a closed building popping out of the ground. The ground face is the outer face.
    /*map->outerFaces.clear();
    auto outerFaces = graphB->getOuterFaces();
    for (size_t fIndex = 0; fIndex < outerFaces.size(); fIndex++) {
        auto* outerFaceB = outerFaces[fIndex];
        auto* outerHalf = outerFaceB->outerComponent;
        int vIndex = graphB->getInterior()->vertexIndex(outerHalf->getVertex());
        map->outerFaces.push_back(map->vertexBtoA[vIndex]->getHalfEdges()[outerHalf->vertexIndex]->getFace());
    }*/
}


Face* MorphismFinder::findFace(FaceType* faceType) {
    if (groundEnabled && faceType == groundFace->getFaceType()) {
        auto preference = globalSettings["Prefer Ground"].get<double>();
        if (Util::randomUniform(0, 1) < preference) {
            return groundFace;
        }
    }

    auto facesA = model->getCurrent()->getFaceMap();
    // TODO: This conversion from map to vector could be slow with many faces.
    // We could just select random samples.
    vector<Face*> facesVec;
    for (const auto& [_, face] : facesA) {
        facesVec.push_back(face);
    }
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

Morphism* MorphismFinder::findStarterMap(Graph* graphB) {
    auto info = make_unique<MorphismInfo>(model, graphB);
    auto state = make_unique<MorphismState>(info.get());

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
    Face* face = findFace(bFaces[0]->getType());
    if (face) {
        state->getMorphism()->faceBtoA = {face};

        // The web version supports cases where we have disconnected boundaries and
        // do a volume splice through castVolumeRaySeries.
        // This was always hacky and so I'm not using it here.
        return state->getMorphism();
    }
    delete state->getMorphism();
    return nullptr;
}

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
        return spliceHalfEdge(halfEdgeData, state);
    }
    return state->getMorphism();
}

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

Morphism* MorphismFinder::matchHalfEdge(
    const HalfEdgeData& halfEdgeData,
    MorphismState* state
) {
    GraphHalfEdge* halfB = halfEdgeData.halfB;
    Vertex* vertexA = halfEdgeData.vertexA;

    auto halfEdgesA = vertexA->getHalfEdges();
    GraphEdge* edgeB = halfB->getEdge();

    if (!edgeB) {
        return findContinue(state);
    }

    EdgeType* typeB = edgeB->getType();

    // Find matching halfEdge.
    HalfEdge* halfEdgeA = nullptr;
    for (auto* halfA : halfEdgesA) {
        if (halfA->getEdgeType() == typeB &&
            halfA->getIsAtStart() == halfB->getForward()) {
            halfEdgeA = halfA;
            break;
        }
    }

    if (!halfEdgeA) {
        return nullptr;
    }

    // Do not use this halfEdge if it is attached to a face with interior vertex and
    // it is not an outer face.
    auto faceB = halfB->getFace();
    auto bFaces = faceB->getGraph()->getBFaces();
    bool isOuter = !faceB->isLoopy() || find(bFaces.begin(), bFaces.end(), faceB) != bFaces.end();
    if (neighboringHoles(halfEdgeA->getEdge(), edgeB) && !isOuter) {
        return nullptr;
    }
    return assignHalfEdge(halfEdgeA, halfB, state);
}

Morphism* MorphismFinder::spliceHalfEdge(
    const HalfEdgeData& halfEdgeData,
    MorphismState* state
) {
    GraphHalfEdge* halfB = halfEdgeData.halfB;
    Vertex* vertexA = halfEdgeData.vertexA;
    Graph* graphB = state->getInfo()->graphB;

    // Find end of splice chain.
    GraphHalfEdge* endB = halfB;
    while (endB->isSpliced()) {
        endB = endB->getNext();
    }

    int vIndexB = indexOf(state->getInfo()->verticesB, endB->getVertex());
    if (state->getMorphism()->vertexBtoA[vIndexB]) {
        return findContinue(state);
    }

    // Find matching halfEdge.
    FaceType* faceTypeB = halfB->getEdge()->getType()->getFaceData()[0].type;
    HalfEdge* halfEdgeA = nullptr;

    for (auto* ep : vertexA->getHalfEdges()) {
        if (ep->getFace()->getFaceType() == faceTypeB) {
            halfEdgeA = ep;
            break;
        }
    }

    if (!halfEdgeA) {
        return nullptr;
    }

    // Cast ray series.
    FaceGroup* groupA = halfEdgeA->getFace()->getGroup();
    int maxDim = faceTypeB->getMaxDim();
    const Vec3 startPos = vertexA->getPosition();

    int attempts = 0;
    HalfEdge* intersectHalfEdge = nullptr;

    while (attempts < spliceRayAttempts && !intersectHalfEdge) {
        intersectHalfEdge = castRaySeries(halfB, startPos, groupA, model->getCurrent()->getFaceMap(), maxDim);
        attempts++;
    }

    if (!intersectHalfEdge) {
        return nullptr;
    }

    // Handle intersection results.
    Vertex* vertexA0 = intersectHalfEdge->getVertex();
    Vertex* vertexA1 = intersectHalfEdge->next()->getVertex();

    int vIndex0 = indexOf(state->getMorphism()->getVertexBtoA(), vertexA0);
    int vIndex1 = indexOf(state->getMorphism()->getVertexBtoA(), vertexA1);
    int eIndex = indexOf(state->getMorphism()->getEdgeBtoA(), intersectHalfEdge->getEdge());

    if (vIndex0 < 0 && vIndex1 < 0 && eIndex < 0) {
        // Split once for unmatched edge and vertices.
        auto result = intersectHalfEdge->getEdge()->fullSplit(Util::randomUniform(0, 1));
        nodesModified = true;
        return assignVertex(state, result.second, vIndexB);
    } else if (vIndex0 >= 0 && vIndex1 >= 0 && eIndex >= 0) {
        // Split three times for matched edge and vertices
        bool isAtStart0 = intersectHalfEdge->getIsAtStart();
        GraphVertex* vertexB0 = graphB->getVertices()[vIndex0];
        GraphVertex* vertexB1 = graphB->getVertices()[vIndex1];

        bool isConnector0 = (vertexB0->boundaryIndex() >= 0);
        bool isConnector1 = (vertexB1->boundaryIndex() >= 0);

        if (!(isConnector0 ^ isConnector1)) {
            cout << "Expected one of the vertices to be a connector." << endl;
        }

        bool connectorAtStart = isConnector0 ? isAtStart0 : !isAtStart0;
        nodesModified = true;

        // Perform triple split
        vector<Edge*> splitEdges;
        vector<Vertex*> splitVertices;
        Edge* edgeToSplit = intersectHalfEdge->getEdge();

        for (int i = 0; i < 3; i++) {
            auto result = edgeToSplit->fullSplit(1.0 / (4 - i));
            splitVertices.push_back(result.second);
            splitEdges.push_back(result.first.edges[0]);
            edgeToSplit = result.first.edges[1];

            if (i == 2) {
                splitEdges.push_back(edgeToSplit);
            }
        }

        int vIndexCon = isConnector0 ? vIndex0 : vIndex1;
        if (connectorAtStart) {
            state->getMorphism()->vertexBtoA[vIndexCon] = splitVertices[2];
            state->getMorphism()->edgeBtoA[eIndex] = splitEdges[3];
            return assignVertex(state, splitVertices[0], vIndexB);
        } else {
            state->getMorphism()->vertexBtoA[vIndexCon] = splitVertices[0];
            state->getMorphism()->edgeBtoA[eIndex] = splitEdges[0];
            return assignVertex(state, splitVertices[2], vIndexB);
        }
    } else if (vIndex0 < 0 || vIndex1 < 0 || eIndex < 0) {
        cout << "Unsure what to do about a partial match" << endl;
    }

    return nullptr;
}

Morphism* MorphismFinder::assignHalfEdge(HalfEdge* halfEdgeA, GraphHalfEdge* halfB, MorphismState* state) {
    auto info = state->getInfo();
    auto morphism = state->getMorphism();

    GraphVertex* vertexB = halfB->getNext()->getVertex();
    int vIndexB = indexOf(info->verticesB, vertexB);

    bool isConnector = vertexB->boundaryIndex() >= 0;
    auto vertexType = vertexB->getType();

    if (!isConnector && vertexType->getSpliced() && morphism->vertexBtoA[vIndexB] == 0) {
        halfEdgeA->getEdge()->fullSplit(random());
        nodesModified = true;
    }

    Edge* edgeA = halfEdgeA->getEdge();
    GraphEdge* edgeB = halfB->getEdge();
    int eIndexB = indexOf(info->edgesB, edgeB);
    morphism->edgeBtoA[eIndexB] = edgeA;

    Vertex* vertexA = halfEdgeA->next()->getVertex();
    int vIndexA = indexOf(morphism->vertexBtoA, vertexA);

    if (vIndexA < morphism->vertexBtoA.size()) {
        if (isConnector || vIndexA == vIndexB) {
            // We've already matched the vertex at the end of halfB.
            return findContinue(state);
        } else {
            // vertexA has been matched somewhere else.
            return nullptr;
        }
    }

    // At this point, halfEdgeA and halfB have no match at their ends.
    if (isConnector || vertexA->getType() == vertexType || vertexType->getSpliced()) {
        return assignVertex(state, vertexA, vIndexB);
    } else {
        return nullptr;
    }
}

// Helper function to find the nearest intersection between a edge and a face
void MorphismFinder::findNearestIntersection(Face* faceA, const Vec3& p0, const Vec3& p1, const Vec2& dir2, IntersectResult& nearestIntersect, int maxDim) {
    vector<Vec3> fPositions = faceA->getPositions();
    vector<IntersectionData> intersections = Intersector::edgeFaceIntersect(p0, p1, fPositions, maxDim);
    
    for (const IntersectionData& intersection : intersections) {
        double distance = dir2.dot(intersection.pos);
        if (distance < nearestIntersect.distance) {
            // Clean up previous data if it exists
            if (nearestIntersect.data != nullptr) {
                delete nearestIntersect.data;
            }
            nearestIntersect.distance = distance;
            nearestIntersect.face = faceA;
            nearestIntersect.data = new IntersectionData(intersection); // Create a copy
        }
    }
}

// Cast a ray and find the nearest intersection
// TODO: Use BSP tree.
IntersectResult MorphismFinder::castRay(const Vec3& p0, const Vec3& dir, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim) {
    Vec3 p1 = dir;
    p1.scale(Intersector::FAR_DISTANCE);
    p1.add(p0);
    Vec2 dir2 = dir.dropDim(maxDim);

    IntersectResult nearestIntersect;
    nearestIntersect.distance = numeric_limits<double>::infinity();
    nearestIntersect.face = nullptr;
    nearestIntersect.data = nullptr;

    for (Face* faceA : groupA->getFaces()) {
        findNearestIntersection(faceA, p0, p1, dir2, nearestIntersect, maxDim);
    }
    // Closed 3D faces should have an intersection here.
    if (nearestIntersect.face) {
        return nearestIntersect;
    }

    // For 2D intersections, search random faces from the map
    int N = (int)faceMap.size();
    int attempts = min(N, faceAttempts);
    int startIndex = Util::randomInt(N);
    
    auto it = next(faceMap.begin(), startIndex % N);
    for (int i = 0; i < attempts; i++) {
        findNearestIntersection(it->second, p0, p1, dir2, nearestIntersect, maxDim);
        
        // Move to next face, wrapping around to beginning if needed
        ++it;
        if (it == faceMap.end()) {
            it = faceMap.begin();
        }
    }
    return nearestIntersect;
}

// Cast a series of rays
HalfEdge* MorphismFinder::castRaySeries(GraphHalfEdge* halfB, const Vec3& startPos, FaceGroup* groupA, const map<int, Face*>& faceMap, int maxDim) {
    Vec3 p0 = startPos;
    GraphHalfEdge* nextB = halfB->getNext();
    
    while (true) {
        const Vec3 dir = halfB->getDir();
        IntersectResult nearestIntersect = castRay(p0, dir, groupA, faceMap, maxDim);

        if (!nearestIntersect.data) {
            return nullptr;
        }
        
        if (nextB->isSpliced()) {
            Vec2 dir2 = dir.dropDim(maxDim);
            double scale = dir.length() / dir2.length();
            double nearestDist = scale * nearestIntersect.distance;
            double maxDistance = max(nearestDist, (double)maxRayDistance);
            double d = Util::randomUniform(0, maxDistance);
            
            p0 = dir;
            p0.scale(d);
            p0.add(startPos);
        } else {
            HalfEdge* nextA = nearestIntersect.face->getHalfEdges()[nearestIntersect.data->index];
            EdgeType* edgeTypeA = nextA->getEdgeType();
            EdgeType* edgeTypeB = nextB->getEdge()->getType();

            delete nearestIntersect.data;
            if (edgeTypeA != edgeTypeB) {
                return nullptr;
            } else {
                return nextA;
            }
        }

        delete nearestIntersect.data; // Clean up
        halfB = nextB;
        nextB = nextB->getNext();
    }
}

