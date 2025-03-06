#include "net_graph_map_finder.h"
#include "net_graph_map.h"
#include "net_graph_map_info.h"
#include "net_graph_map_state.h"
//#include "face.h"
//#include "vertex.h"
//#include "half_edge.h"
//#include "vertex_type.h"
#include "../shapes3D/edge_type3d.h"
//#include "face_group.h"
//#include "line.h"
//#include "edge.h"
#include "../util/util.h"
//#include "settings.h"

namespace ms {

std::unordered_map<int, VertexType*> NetGraphMapFinder::splicedVertexTypes;

NetGraphMapFinder::NetGraphMapFinder(Model* model, bool groundEnabled)
    : model(model)
    , nodesModified(false)
    , groundFace(nullptr)
    , groundEnabled(groundEnabled) {}

NetGraphMap* NetGraphMapFinder::findMap(Network* netB) {
    /*if (!groundFace && groundEnabled) {
        auto faces = model->getCurrent()->getFaces();
        if (!faces.empty()) {
            groundFace = faces[0];
        }
    }*/

    auto verticesB = netB->getVertices();
    if (verticesB.empty()) {
        return findStarterMap(netB);
    }

    // TODO: Handle non-starter transitions.

    // Start with a vertex that is not spliced
    int index1 = 0;
    while (verticesB[index1]->getType()->getSpliced()) {
        index1++;
    }
    //// auto* primalB = verticesB[index1]->getPrimal();

    bool isConnector = verticesB[index1]->connectorIndex() >= 0;
    auto* vertexType = verticesB[index1]->getType();
    auto vertexMap = model->getCurrent()->getVertexMap();
    int N = vertexMap.size();
    int attempts = std::min(N, vertexAttempts);
    int startIndex = Util::random(N);

    auto it = std::next(vertexMap.begin(), startIndex % vertexMap.size()); // Start at valid index
    size_t count = 0;

    while (count < attempts) {
        auto vertexA = it->second;
        if ((isConnector || vertexA->getType() == vertexType)
            /* && !vertexA->getNode()->isDestroyed() */) {
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

        ++count;

        // Move to the next element
        ++it;

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
        map->outerFaces.push_back(map->vertexBtoA[vIndex]->getEndpoints()[outerHalf->vertexIndex]->getFace());
    }*/
}

//
//Face* NetGraphMapFinder::findFace(VertexType* faceType) {
//    if (groundEnabled && faceType == groundFace->getFaceType()) {
//        if (Util::randomUniform(0, 1) < Settings::get("Prefer Ground")) {
//            return groundFace;
//        }
//    }
//
//    auto facesA = nodeStats->getElements("face");
//    int N = facesA.size();
//    int attempts = std::min(N, faceAttempts);
//    int startIndex = Util::random(N);
//    std::vector<Face*> options;
//    std::vector<float> weights;
//
//    for (int i = 0; i < attempts; i++) {
//        auto* faceA = facesA[(startIndex + i) % N];
//        if (faceA->getFaceType() == faceType && !faceA->isHole()) {
//            weights.push_back(std::abs(faceA->signedArea()));
//            options.push_back(faceA);
//        }
//    }
//
//    if (!weights.empty()) {
//        return options[Util::pickByWeight(weights)];
//    }
//    return nullptr;
//}

NetGraphMap* NetGraphMapFinder::findStarterMap(Network* netB) {
    auto info = std::make_unique<NetGraphMapInfo>(model, netB);
    auto state = std::make_unique<NetGraphMapState>(info.get());

    // auto edges = netB->getBoundary()->getEdges();
    auto faces = netB->getFaces();
    if (faces.empty()) {
        return state->getMap();
    }

    /*if (!groundEnabled || nodeStats->getCount("face") == 0) {
        return state->map;
    }

    Face* face = findFace(edges[0]->getPrimal()->getType());
    if (face) {
        state->map->outerFaces = { face };
        if (!netB->boundary->faces[0]->innerComponents.empty()) {
            Face* goalFace = netB->boundary->faces[1];
            Face* intersectFace = nullptr;
            int attempts = 0;

            while (attempts < spliceRayAttempts && !intersectFace) {
                intersectFace = castVolumeRaySeries(face,
                    netB->boundary->faces[0]->innerComponents,
                    goalFace);
                attempts++;
            }

            if (!intersectFace) {
                return nullptr;
            }
            else {
                state->map->outerFaces.push_back(intersectFace);
            }
        }
        return state->map;
    }*/
    return nullptr;
}

//Face* NetGraphMapFinder::castVolumeRaySeries(Face* face,
//    const std::vector<HalfEdge*>& rayHalfs, Face* goalFace) {
//
//    if (rayHalfs.size() > 1) {
//        // TODO: Implement multiple rays
//        throw std::runtime_error("Multiple Rays not implemented yet.");
//    }
//
//    Vector3 startPos = face->randomPoint();
//    BspTree* tree = nodeStats->model->getBspTree();
//    EdgeType* goalType = goalFace->outerComponent->edge->primal->type;
//
//    // Model ray as long polygon and intersect with BSP tree polygons
//    FaceType* faceType = face->getFaceType();
//    Vector3 dir = rayHalfs[0]->getDir();
//    Vector3 u = faceType->u;
//    Vector3 v = faceType->v;
//
//    Vector3 endPos = dir * Intersector::FAR_DISTANCE + startPos;
//    Vector3 startPos2 = u * SMALL_DISTANCE + startPos;
//    Vector3 endPos2 = u * SMALL_DISTANCE + endPos;
//
//    std::vector<Vector3> points = { startPos, endPos, endPos2, startPos2 };
//    double d = v.dot(startPos);
//
//    BspPlane plane(v, d);
//    auto fakePolygon = std::make_shared<Polygon>(points);
//
//    auto intersections = tree->add(plane, fakePolygon, true);
//
//    // Find closest intersection
//    double closestDist = std::numeric_limits<double>::infinity();
//    Intersection* closest = nullptr;
//
//    for (const auto& intersection : intersections) {
//        double d = dir.dot(intersection->position);
//        if (d < closestDist) {
//            closest = intersection;
//            closestDist = d;
//        }
//    }
//
//    if (closest) {
//        Face* nextFace = closest->polygon->getFace();
//        if (nextFace->getFaceType() == goalType && !nextFace->isHole()) {
//            return nextFace;
//        }
//    }
//
//    return nullptr;
//}

NetGraphMap* NetGraphMapFinder::findContinue(NetGraphMapState* state) {
    if (!state->getQueue().empty()) {
        std::vector<EndpointData>& queue = state->getQueue();
        EndpointData endpointData = queue.front();
        queue.erase(queue.begin());
        return matchEndpoint(endpointData, state);
    } /* else if (!state->getSpliceQueue().empty()) {
        auto queue = state->getSpliceQueue();
        EndpointData endpointData = queue.front();
        queue.erase(queue.begin());
        return spliceEndpoint(endpointData, state);
    } */
    return state->getMap();
}

//bool neighboringHoles(const Line& line, const EdgeNet& edgeB) {
//    const auto& endpoints = line.getEndpoints();
//    const auto& halfEdges = edgeB.getHalfEdges();
//
//    return std::any_of(endpoints.begin(), endpoints.end(),
//        [&](const Endpoint* endpoint) {
//            Face* face = endpoint->getFace();
//            if (!face) return false;
//
//            bool hasHoles = (face->getGroupFaces().size() > 1) && !face->isHole();
//            size_t index = &endpoint - &endpoints[0]; // Get index of endpoint
//            return hasHoles && halfEdges[index][0]->isLoopy();
//        }
//    );
//}

NetGraphMap* NetGraphMapFinder::matchEndpoint(
    const EndpointData& endpointData,
    NetGraphMapState* state
) {
    HalfEdgeNet* halfB = endpointData.halfB;
    Vertex* vertexA = endpointData.vertexA;

    auto endpointsA = vertexA->getEndpoints();
    EdgeNet* edgeB = halfB->getEdge();

    if (!edgeB) {
        return findContinue(state);
    }

    EdgeType3D* typeB = edgeB->getType();

    // Find matching endpoint
    Endpoint* endpointA = nullptr;
    for (auto* ep : endpointsA) {
        if (ep->getEdgeType() == typeB &&
            ep->getIsAtStart() == halfB->getForward()) {
            endpointA = ep;
            break;
        }
    }

    if (!endpointA) {
        return nullptr;
    }

    // Check neighboring holes condition
    /*if (neighboringHoles(endpointA->getLine(), edgeB) &&
        halfB->getFace()->getPrimal()->getTurns() != 1) {
        return nullptr;
    }*/

    return assignEndpoint(endpointA, halfB, state);
}

//NetGraphMap* NetGraphMapFinder::spliceEndpoint(const EndpointData& endpointData,
//    NetGraphMapState* state) {
//
//    HalfEdge* halfB = endpointData.halfB;
//    Vertex* vertexA = endpointData.vertexA;
//    NetB* netB = state->info->netB;
//
//    // Find end of splice chain
//    HalfEdge* endB = halfB;
//    while (endB->isSpliced()) {
//        endB = endB->getNext();
//    }
//
//    int vIndexB = state->info->findVertexIndex(endB->getVertex());
//    if (state->map->vertexBtoA[vIndexB]) {
//        return findContinue(state);
//    }
//
//    // Find matching endpoint
//    EdgeType* faceTypeB = halfB->getEdge()->getPrimal()->getType()->getFaceData()[0].type;
//    Endpoint* endpointA = nullptr;
//
//    for (auto* ep : vertexA->getEndpoints()) {
//        if (ep->getFace()->getFaceType() == faceTypeB) {
//            endpointA = ep;
//            break;
//        }
//    }
//
//    if (!endpointA) {
//        return nullptr;
//    }
//
//    // Cast ray series
//    FaceGroup* groupA = endpointA->getFace()->getGroup();
//    int maxDim = faceTypeB->getMaxDim();
//    Vector3 startPos = vertexA->getPosition();
//
//    int attempts = 0;
//    Endpoint* intersectEndpoint = nullptr;
//
//    while (attempts < spliceRayAttempts && !intersectEndpoint) {
//        intersectEndpoint = castRaySeries(halfB, startPos, groupA, maxDim);
//        attempts++;
//    }
//
//    if (!intersectEndpoint) {
//        return nullptr;
//    }
//
//    // Handle intersection results
//    Vertex* vertexA0 = intersectEndpoint->getVertex();
//    Vertex* vertexA1 = intersectEndpoint->next()->getVertex();
//
//    int vIndex0 = state->map->findVertexIndex(vertexA0);
//    int vIndex1 = state->map->findVertexIndex(vertexA1);
//    int eIndex = state->map->findEdgeIndex(intersectEndpoint->getLine());
//
//    if (vIndex0 < 0 && vIndex1 < 0 && eIndex < 0) {
//        // Split once for unmatched line and vertices
//        auto result = intersectEndpoint->getLine()->fullSplit(Util::randomUniform(0, 1));
//        nodesModified = true;
//        return assignVertex(state, result.newVertex, vIndexB);
//    }
//    else if (vIndex0 >= 0 && vIndex1 >= 0 && eIndex >= 0) {
//        // Split three times for matched line and vertices
//        bool isAtStart0 = intersectEndpoint->getIsAtStart();
//        Vertex* vertexB0 = netB->getInterior()->getVertices()[vIndex0];
//        Vertex* vertexB1 = netB->getInterior()->getVertices()[vIndex1];
//
//        bool isConnector0 = (vertexB0->getPrimal()->connectorIndex() >= 0);
//        bool isConnector1 = (vertexB1->getPrimal()->connectorIndex() >= 0);
//
//        if (!(isConnector0 ^ isConnector1)) {
//            throw std::runtime_error("Expected one of the vertices to be a connector.");
//        }
//
//        bool connectorAtStart = isConnector0 ? isAtStart0 : !isAtStart0;
//        nodesModified = true;
//
//        // Perform triple split
//        std::vector<Line*> splitLines;
//        std::vector<Vertex*> splitVertices;
//        Line* lineToSplit = intersectEndpoint->getLine();
//
//        for (int i = 0; i < 3; i++) {
//            auto result = lineToSplit->fullSplit(1.0 / (4 - i));
//            splitVertices.push_back(result.newVertex);
//            splitLines.push_back(result.split.lines[0]);
//            lineToSplit = result.split.lines[1];
//
//            if (i == 2) {
//                splitLines.push_back(lineToSplit);
//            }
//        }
//
//        int vIndexCon = isConnector0 ? vIndex0 : vIndex1;
//        if (connectorAtStart) {
//            state->map->vertexBtoA[vIndexCon] = splitVertices[2];
//            state->map->edgeBtoA[eIndex] = splitLines[3];
//            return assignVertex(state, splitVertices[0], vIndexB);
//        }
//        else {
//            state->map->vertexBtoA[vIndexCon] = splitVertices[0];
//            state->map->edgeBtoA[eIndex] = splitLines[0];
//            return assignVertex(state, splitVertices[2], vIndexB);
//        }
//    }
//    else if (vIndex0 < 0 || vIndex1 < 0 || eIndex < 0) {
//        throw std::runtime_error("Unsure what to do about a partial match");
//    }
//
//    return nullptr;
//}

NetGraphMap* NetGraphMapFinder::assignEndpoint(Endpoint* endpointA, HalfEdgeNet* halfB, NetGraphMapState* state) {
    auto info = state->getInfo();
    auto map = state->getMap();

    VertexNet* vertexB = halfB->getNext()->getVertex();
    int vIndexB = std::distance(info->verticesB.begin(),
        std::find(info->verticesB.begin(), info->verticesB.end(), vertexB));

    bool isConnector = vertexB->connectorIndex() >= 0;
    auto vertexType = vertexB->getType();

    //if (!isConnector && vertexType.getSpliced() && map->vertexBtoA[vIndexB] == 0) {
    //    endpointA->getLine()->fullSplit(static_cast<double>(rand()) / RAND_MAX); // Simulating ms.randomUniform(0, 1)
    //    nodesModified = true;
    //}

    Line* lineA = endpointA->getLine();
    EdgeNet* edgeB = halfB->getEdge();
    int eIndexB = std::distance(info->edgesB.begin(),
        std::find(info->edgesB.begin(), info->edgesB.end(), edgeB));
    map->edgeBtoA[eIndexB] = lineA;

    Vertex* vertexA = endpointA->next()->getVertex();
    int vIndexA = std::distance(map->vertexBtoA.begin(),
        std::find(map->vertexBtoA.begin(), map->vertexBtoA.end(), vertexA));

    if (vIndexA < map->vertexBtoA.size()) {
        if (isConnector || vIndexA == vIndexB) {
            // We've already matched the vertex at the end of halfB.
            return findContinue(state);
        }
        else {
            // vertexA has been matched somewhere else.
            return nullptr;
        }
    }

    // At this point, endpointA and halfB have no match at their ends.
    if (isConnector || vertexA->getType() == vertexType || vertexType->getSpliced()) {
        return assignVertex(state, vertexA, vIndexB);
    } else {
        return nullptr;
    }
}

} // namespace ms 