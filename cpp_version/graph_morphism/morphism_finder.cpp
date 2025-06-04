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

namespace ms {

std::unordered_map<int, VertexType*> NetGraphMapFinder::splicedVertexTypes;

NetGraphMapFinder::NetGraphMapFinder(Model* model, bool groundEnabled)
    : model(model)
    , nodesModified(false)
    , groundFace(nullptr)
    , groundEnabled(groundEnabled) {}

void NetGraphMapFinder::reset() {
    nodesModified = false;
    groundFace = nullptr;
}

NetGraphMap* NetGraphMapFinder::findMap(Network* netB) {
    if (!groundFace && groundEnabled) {
        auto faces = model->getCurrent()->getFaceMap();
        if (!faces.empty()) {
            groundFace = faces.begin()->second;
        }
    }

    auto verticesB = netB->getVertices();
    if (verticesB.empty()) {
        return findStarterMap(netB);
    }

    // Start with a vertex that is not spliced
    int index1 = 0;
    while (verticesB[index1]->getType()->getSpliced()) {
        index1++;
    }

    bool isConnector = verticesB[index1]->connectorIndex() >= 0;
    auto* vertexType = verticesB[index1]->getType();
    auto vertexMap = model->getCurrent()->getVertexMap();
    int N = (int)vertexMap.size();
    if (N == 0) {
        return nullptr;
    }
    int attempts = min(N, vertexAttempts);
    int startIndex = Util::randomInt(N);

    auto it = std::next(vertexMap.begin(), startIndex % vertexMap.size()); // Start at valid index
    size_t count = 0;

    while (count < attempts) {
        auto vertexA = it->second;
        if ((isConnector || vertexA->getType() == vertexType)) {
            nodesModified = false;
            auto* info = new NetGraphMapInfo(model, netB);
            auto* state = new NetGraphMapState(info);
            auto* map = assignVertex(state, vertexA, index1);
                
            if (map) {
                addOuterFaces(map, netB);
                delete state;
                delete info;
                return map;
            } else if (nodesModified) {
                model->reject();
            }
                
            delete state;
            delete info;
        }

        count++;

        // Move to the next element
        it++;

        // If we reach the end, roll over to the beginning
        if (it == vertexMap.end()) {
            it = vertexMap.begin();
        }
    }
    return nullptr;
}


NetGraphMap* NetGraphMapFinder::assignVertex(NetGraphMapState* state, Vertex* vertexA, int indexB) {
    state->assignVertex(vertexA, indexB);
    return findContinue(state);
}

void NetGraphMapFinder::addOuterFaces(NetGraphMap* map, Network* netB) {
    // TODO: Implement this. For a closed building popping out of the ground. The ground face is the outer face.
    /*map->outerFaces.clear();
    auto outerFaces = netB->getOuterFaces();
    for (size_t fIndex = 0; fIndex < outerFaces.size(); fIndex++) {
        auto* outerFaceB = outerFaces[fIndex];
        auto* outerHalf = outerFaceB->outerComponent;
        int vIndex = netB->getInterior()->vertexIndex(outerHalf->getVertex());
        map->outerFaces.push_back(map->vertexBtoA[vIndex]->getHalfEdges()[outerHalf->vertexIndex]->getFace());
    }*/
}


Face* NetGraphMapFinder::findFace(FaceType3D* faceType) {
    if (groundEnabled && faceType == groundFace->getFaceType()) {
        auto preference = globalSettings["Prefer Ground"].get<double>();
        if (Util::randomUniform(0, 1) < preference) {
            return groundFace;
        }
    }

    auto facesA = model->getCurrent()->getFaceMap();
    // TODO: This conversion from map to vector could be slow with many faces.
    // We could just select random samples.
    std::vector<Face*> facesVec;
    for (const auto& [_, face] : facesA) {
        facesVec.push_back(face);
    }
    int N = (int)facesVec.size();
    int attempts = min(N, faceAttempts);
    int startIndex = Util::randomInt(N);
    std::vector<Face*> options;
    std::vector<double> weights;

    for (int i = 0; i < attempts; i++) {
        auto* faceA = facesVec[(startIndex + i) % N];
        if (faceA && faceA->getFaceType() == faceType && !faceA->isHole()) {
            weights.push_back(std::abs(faceA->signedArea()));
            options.push_back(faceA);
        }
    }

    if (!weights.empty()) {
        return options[Util::randomDistribution(weights)];
    }
    return nullptr;
}

NetGraphMap* NetGraphMapFinder::findStarterMap(Network* netB) {
    auto info = std::make_unique<NetGraphMapInfo>(model, netB);
    auto state = std::make_unique<NetGraphMapState>(info.get());

    auto faces = netB->getFaces();
    if (faces.empty()) {
        return state->getMap();
    }

    auto faceMap = model->getCurrent()->getFaceMap();
    if (!groundEnabled || faceMap.empty()) {
        return state->getMap();
    }

    auto bFaces = netB->getBFaces();
    if (bFaces.size() != 1) {
        std::cout << "Expect one boundary face." << std::endl;
        return nullptr;
    }
    Face* face = findFace(bFaces[0]->getType());
    if (face) {
        state->getMap()->faceBtoA = {face};

        // The web version supports cases where we have disconnected boundaries and
        // do a volume splice through castVolumeRaySeries.
        // This was always hacky and so I'm not using it here.
        return state->getMap();
    }
    return nullptr;
}

NetGraphMap* NetGraphMapFinder::findContinue(NetGraphMapState* state) {
    if (!state->getQueue().empty()) {
        std::vector<HalfEdgeData>& queue = state->getQueue();
        HalfEdgeData halfedgeData = queue.front();
        queue.erase(queue.begin());
        return matchHalfEdge(halfedgeData, state);
    } else if (!state->getSpliceQueue().empty()) {
        std::vector<HalfEdgeData>& queue = state->getSpliceQueue();
        HalfEdgeData halfedgeData = queue.front();
        queue.erase(queue.begin());
        return spliceHalfEdge(halfedgeData, state);
    }
    return state->getMap();
}

bool neighboringHoles(Edge* edge, EdgeNet* edgeB) {
    const auto& halfedges = edge->getHalfEdges();
    const auto& halfEdges = edgeB->getHalfEdges();

    for (int i = 0; i < halfedges.size(); i++) {
        auto halfedge = halfedges[i];
        Face* face = halfedge->getFace();
        bool hasHoles = (face->getGroup()->getFaces().size() > 1) && !face->isHole();
        if (hasHoles && halfEdges[i][0]->isLoopy()) {
            return true;
        }
    }
    return false;
}

NetGraphMap* NetGraphMapFinder::matchHalfEdge(
    const HalfEdgeData& halfedgeData,
    NetGraphMapState* state
) {
    HalfEdgeNet* halfB = halfedgeData.halfB;
    Vertex* vertexA = halfedgeData.vertexA;

    auto halfedgesA = vertexA->getHalfEdges();
    EdgeNet* edgeB = halfB->getEdge();

    if (!edgeB) {
        return findContinue(state);
    }

    EdgeType3D* typeB = edgeB->getType();

    // Find matching halfedge.
    HalfEdge* halfedgeA = nullptr;
    for (auto* ep : halfedgesA) {
        if (ep->getEdgeType() == typeB &&
            ep->getIsAtStart() == halfB->getForward()) {
            halfedgeA = ep;
            break;
        }
    }

    if (!halfedgeA) {
        return nullptr;
    }

    // Do not use this halfedge if it is attached to a face with interior vertex and
    // it is not an outer face.
    auto faceB = halfB->getFace();
    auto bFaces = faceB->getNetwork()->getBFaces();
    bool isOuter = !faceB->isLoopy() || std::find(bFaces.begin(), bFaces.end(), faceB) != bFaces.end();
    if (neighboringHoles(halfedgeA->getEdge(), edgeB) && !isOuter) {
        return nullptr;
    }
    return assignHalfEdge(halfedgeA, halfB, state);
}

NetGraphMap* NetGraphMapFinder::spliceHalfEdge(
    const HalfEdgeData& halfedgeData,
    NetGraphMapState* state
) {
    HalfEdgeNet* halfB = halfedgeData.halfB;
    Vertex* vertexA = halfedgeData.vertexA;
    Network* netB = state->getInfo()->networkB;

    // Find end of splice chain.
    HalfEdgeNet* endB = halfB;
    while (endB->isSpliced()) {
        endB = endB->getNext();
    }

    int vIndexB = indexOf(state->getInfo()->verticesB, endB->getVertex());
    if (state->getMap()->vertexBtoA[vIndexB]) {
        return findContinue(state);
    }

    // Find matching halfedge.
    FaceType3D* faceTypeB = halfB->getEdge()->getType()->getFaceData()[0].type;
    HalfEdge* halfedgeA = nullptr;

    for (auto* ep : vertexA->getHalfEdges()) {
        if (ep->getFace()->getFaceType() == faceTypeB) {
            halfedgeA = ep;
            break;
        }
    }

    if (!halfedgeA) {
        return nullptr;
    }

    // Cast ray series.
    FaceGroup* groupA = halfedgeA->getFace()->getGroup();
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

    int vIndex0 = indexOf(state->getMap()->getVertexBtoA(), vertexA0);
    int vIndex1 = indexOf(state->getMap()->getVertexBtoA(), vertexA1);
    int eIndex = indexOf(state->getMap()->getEdgeBtoA(), intersectHalfEdge->getEdge());

    if (vIndex0 < 0 && vIndex1 < 0 && eIndex < 0) {
        // Split once for unmatched edge and vertices.
        auto result = intersectHalfEdge->getEdge()->fullSplit(Util::randomUniform(0, 1));
        nodesModified = true;
        return assignVertex(state, result.second, vIndexB);
    } else if (vIndex0 >= 0 && vIndex1 >= 0 && eIndex >= 0) {
        // Split three times for matched edge and vertices
        bool isAtStart0 = intersectHalfEdge->getIsAtStart();
        VertexNet* vertexB0 = netB->getVertices()[vIndex0];
        VertexNet* vertexB1 = netB->getVertices()[vIndex1];

        bool isConnector0 = (vertexB0->connectorIndex() >= 0);
        bool isConnector1 = (vertexB1->connectorIndex() >= 0);

        if (!(isConnector0 ^ isConnector1)) {
            std::cout << "Expected one of the vertices to be a connector." << std::endl;
        }

        bool connectorAtStart = isConnector0 ? isAtStart0 : !isAtStart0;
        nodesModified = true;

        // Perform triple split
        std::vector<Edge*> splitEdges;
        std::vector<Vertex*> splitVertices;
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
            state->getMap()->vertexBtoA[vIndexCon] = splitVertices[2];
            state->getMap()->edgeBtoA[eIndex] = splitEdges[3];
            return assignVertex(state, splitVertices[0], vIndexB);
        } else {
            state->getMap()->vertexBtoA[vIndexCon] = splitVertices[0];
            state->getMap()->edgeBtoA[eIndex] = splitEdges[0];
            return assignVertex(state, splitVertices[2], vIndexB);
        }
    } else if (vIndex0 < 0 || vIndex1 < 0 || eIndex < 0) {
        std::cout << "Unsure what to do about a partial match" << std::endl;
    }

    return nullptr;
}

NetGraphMap* NetGraphMapFinder::assignHalfEdge(HalfEdge* halfedgeA, HalfEdgeNet* halfB, NetGraphMapState* state) {
    auto info = state->getInfo();
    auto map = state->getMap();

    VertexNet* vertexB = halfB->getNext()->getVertex();
    int vIndexB = indexOf(info->verticesB, vertexB);

    bool isConnector = vertexB->connectorIndex() >= 0;
    auto vertexType = vertexB->getType();

    if (!isConnector && vertexType->getSpliced() && map->vertexBtoA[vIndexB] == 0) {
        halfedgeA->getEdge()->fullSplit(random());
        nodesModified = true;
    }

    Edge* edgeA = halfedgeA->getEdge();
    EdgeNet* edgeB = halfB->getEdge();
    int eIndexB = indexOf(info->edgesB, edgeB);
    map->edgeBtoA[eIndexB] = edgeA;

    Vertex* vertexA = halfedgeA->next()->getVertex();
    int vIndexA = indexOf(map->vertexBtoA, vertexA);

    if (vIndexA < map->vertexBtoA.size()) {
        if (isConnector || vIndexA == vIndexB) {
            // We've already matched the vertex at the end of halfB.
            return findContinue(state);
        } else {
            // vertexA has been matched somewhere else.
            return nullptr;
        }
    }

    // At this point, halfedgeA and halfB have no match at their ends.
    if (isConnector || vertexA->getType() == vertexType || vertexType->getSpliced()) {
        return assignVertex(state, vertexA, vIndexB);
    } else {
        return nullptr;
    }
}

// Helper function to find the nearest intersection between a edge and a face
void NetGraphMapFinder::findNearestIntersection(Face* faceA, const Vec3& p0, const Vec3& p1, const Vec2& dir2, IntersectResult& nearestIntersect, int maxDim) {
    std::vector<Vec3> fPositions = faceA->getPositions();
    std::vector<IntersectionData> intersections = Intersector::edgeFaceIntersect(p0, p1, fPositions, maxDim);
    
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
IntersectResult NetGraphMapFinder::castRay(const Vec3& p0, const Vec3& dir, FaceGroup* groupA, const std::map<int, Face*>& faceMap, int maxDim) {
    Vec3 p1 = dir;
    p1.scale(Intersector::FAR_DISTANCE);
    p1.add(p0);
    Vec2 dir2 = dir.dropDim(maxDim);

    IntersectResult nearestIntersect;
    nearestIntersect.distance = std::numeric_limits<double>::infinity();
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
    
    auto it = std::next(faceMap.begin(), startIndex % N);
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
HalfEdge* NetGraphMapFinder::castRaySeries(HalfEdgeNet* halfB, const Vec3& startPos, FaceGroup* groupA, const std::map<int, Face*>& faceMap, int maxDim) {
    Vec3 p0 = startPos;
    HalfEdgeNet* nextB = halfB->getNext();
    
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
            EdgeType3D* edgeTypeA = nextA->getEdgeType();
            EdgeType3D* edgeTypeB = nextB->getEdge()->getType();

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

} // namespace ms 