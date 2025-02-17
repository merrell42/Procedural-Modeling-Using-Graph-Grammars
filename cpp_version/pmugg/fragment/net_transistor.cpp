#include "net_transistor.h"
#include "../network/network.h"
// #include "transition.h"
// #include "graph.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/line.h"
#include "../graph_drawing/face.h"
#include "../util/timer.h"
#include "../util/util.h"
#include <random>
#include <iostream>
#include <set>

namespace ms {

std::unique_ptr<NetTransistor> NetTransistor::buildNormally(
    const Transition& transition, Model* model) {
    
    auto result = std::make_unique<NetTransistor>();
    result->create(transition, model);
    
    if (result->effort > 0) {
        result->reject();
        return nullptr;
    }
    return result;
}

void NetTransistor::create(const Transition& transition, Model* model_) {
    startNet = transition.startNet;
    model = model_;
    // endNet = transition.endNet->removeSplices();
    endNet = transition.endNet;
    // map = std::move(transition.map);
    map = transition.map;
    ground = transition.ground;
    // dims = is3D ? 3 : 2;
    dims = 3;
    
    // isInitialBoundary = transition.initialBoundary;
    effort = 0;

    timer->start("Create Graph");
    // TODO: Merge duplicate lines
    /*if (!mergeDuplicateLines()) {
        effort = std::numeric_limits<double>::infinity();
        return;
    }*/
    graph = createGraph();
    timer->stop("Create Graph");

    if (!graph) {
        effort = std::numeric_limits<double>::infinity();
        return;
    }

    for (auto* edge : graph->edges) {
        addLine(edge, false, false);
    }
}

void NetTransistor::addLine(Line* line, bool includeLength, bool addToGraph) {
    float angle = line->getEdgeType()->getAngle();
    
    LineData datum;
    datum.v = Vec2::unitVec(angle);
    datum.line = line;
    
    auto lineEndpoints = line->getEndpoints();
    if (includeLength) {
        Vec3 v = lineEndpoints[1]->getPosition() - lineEndpoints[0]->getPosition();
        datum.length = v.length();
    }
    
    lineData.push_back(datum);
    lines.push_back(line);

    if (addToGraph) {
        for (auto* endpoint : lineEndpoints) {
            Util::union_(graph->vertices, {endpoint->getVertex()});
        }
        graph->edges.push_back(line);
    }
}

// Helper function to add connections to the set
//void addConnectionsToSet(Endpoint* endpoint, std::set<Connection*>& connectionsToUpdate) {
//    if (endpoint) {
//        for (auto* connection : endpoint->getConnections()) {
//            connectionsToUpdate.insert(connection);
//        }
//    }
//}

Graph* NetTransistor::createGraph() {
    auto* endNetwork = endNet;
    auto& endVertices = endNetwork->getVertices();
    auto& endEdges = endNetwork->getEdges();
    Graph merged;
    
    // Save face locations
    /*if (map->outerFaces) {
        map->outerFacesD.clear();
        for (auto* outerFace : map->outerFaces) {
            map->outerFacesD.push_back(
                outerFace->getFaceType()->getNormal().dot(
                    outerFace->getEndpoints()[0]->getPosition()));
        }
    }*/

    // Maps from half edges to merge endpoints
    std::vector<Endpoint*> mergedEndpoints(endNetwork->getHalfEdges().size());
    std::fill(mergedEndpoints.begin(), mergedEndpoints.end(), nullptr);
    auto halfToEndpoint = [&](HalfEdgeNet* half) -> Endpoint* {
        int index = Util::findIndex<HalfEdgeNet*>(endNetwork->getHalfEdges(), half);
        return mergedEndpoints[index];
    };
    
    auto setHalfToEndpoint = [&](HalfEdgeNet* half, Endpoint* endpoint) {
        int index = Util::findIndex<HalfEdgeNet*>(endNetwork->getHalfEdges(), half);
        mergedEndpoints[index] = endpoint;
    };

    std::vector<std::vector<Line*>> splitLines;
    std::vector<std::vector<Endpoint*>> splitEndpoints;
    if (map) {
        splitLines.resize(map->edgeBtoA.size());
        for (size_t i = 0; i < map->edgeBtoA.size(); ++i) {
            splitLines[i] = {map->edgeBtoA[i]};
        }
    }

    // Create vertices
    const int numVertices = endVertices.size();
    merged.vertices.resize(numVertices, nullptr);
    for (size_t i = 0; i < numVertices; ++i) {
        auto* v = endVertices[i];

        if (v->connectorIndex() < 0) {
            auto* type = v->getType();
            
            // Create random position based on dimensions
            Vec3 randomPosition;
            /*if (dims == 2) {
                randomPosition = Vec3(5.0 * rand() / RAND_MAX, 
                                   5.0 * rand() / RAND_MAX, 0);
            } else {*/
                randomPosition = Vec3(5.0 * rand() / RAND_MAX,
                                   5.0 * rand() / RAND_MAX,
                                   5.0 * rand() / RAND_MAX);
            // }

            auto* newVertex = new Vertex(model, randomPosition, type);
            newVertex->createEndpoints();
            merged.vertices[i] = newVertex;
            
            auto vEndpoints = newVertex->getEndpoints();
            auto& halfs = v->getHalfEdges();
            
            for (size_t j = 0; j < halfs.size(); ++j) {
                setHalfToEndpoint(halfs[j], vEndpoints[j]);
            }
        }
    }

    struct EdgeData {
        std::vector<Endpoint*> coreEndpoints;
        std::vector<HalfEdgeNet*> halfEdges;
        bool modified;
    };

    // Process edges
    std::vector<EdgeData> edgeData;
    
    for (size_t i = 0; i < endEdges.size(); ++i) {
        auto* endEdge = endEdges[i];
        auto& edgeHalfs = endEdge->getHalfEdges();
        std::vector<Endpoint*> coreEndpoints;
        std::vector<HalfEdgeNet*> halfEdges;
        bool modified = false;

        for (size_t e = 0; e < edgeHalfs.size(); ++e) {
            auto* half = edgeHalfs[e][0];
            auto* hVertex = half->getVertex();
            int hIndex = Util::findIndex(endVertices, hVertex);
            auto* core = merged.vertices[hIndex];

            int connectorIndex = hVertex->connectorIndex();
            if (connectorIndex >= 0) {
                std::cout << "Add connectors" << std::endl;
                int startIndex = 0;
                /*int startIndex = Util::findIndex(startNet->getEdges(),
                    startNet->getConnectors()[connectorIndex]->interiorEdge());*/
                
                if (splitLines[startIndex].size() == 1) {
                    //if (splitLines[startIndex][0]->getNode()->isDestroyed()) {
                    //    // Error handling
                    //    return nullptr;
                    //}
                    auto split = splitLines[startIndex][0]->split();
                    splitLines[startIndex] = split.lines;
                    splitEndpoints[startIndex] = split.nextEndpoints;
                }

                int lineIndex = half->getForward() ? 0 : 1;
                auto* coreEndpoint = splitLines[startIndex][lineIndex]->getEndpoints()[e];
                splitLines[startIndex][lineIndex] = nullptr;
                coreEndpoints[e] = coreEndpoint;

                halfEdges.push_back(half);
                modified = true;
                setHalfToEndpoint(half, coreEndpoint);
            } else {
                auto* coreEndpoint = core->getEndpoints()[half->getVertexIndex()];
                coreEndpoints.push_back(coreEndpoint);
                halfEdges.push_back(half);
            }
        }
        
        edgeData.push_back({coreEndpoints, halfEdges, modified});
    }

    // Process end edges
    //for (size_t i = 0; i < endEdges.size(); i++) {
    //    EdgeNet* endEdge = endEdges[i];
    //    auto edgeHalfs = endEdge->getHalfEdges();

    //    for (size_t e = 0; e < edgeHalfs.size(); e++) {
    //        HalfEdgeNet* halfNext = edgeHalfs[e][0]->getNext();
    //        if (!halfNext->getEdge()) {
    //            VertexNet* hVertex = halfNext->getVertex();
    //            int connectorIndex = hVertex->connectorIndex();

    //            // TODO: Add connectors
    //            auto& startNetEdges = startNet->getEdges();
    //            auto& startNetConnectors = startNet->getConnectors();

    //            auto it = std::find(startNetEdges.begin(), startNetEdges.end(),
    //                startNetConnectors[connectorIndex]->interiorEdge());
    //            int index = std::distance(startNetEdges.begin(), it);

    //            setHalfToEndpoint(halfNext, splitEndpoints[index][e]);
    //        }
    //    }
    //}

    bool failed = false;

    // Process faces in end network
    for (FaceNet* face : endNetwork->getFaces()) {
        auto halfs = face->getOuterHalfEdges();
        size_t N = halfs.size();

        for (size_t i = 0; i < N; i++) {
            HalfEdgeNet* halfA = const_cast<HalfEdgeNet*>(halfs[i]);
            HalfEdgeNet* halfB = const_cast<HalfEdgeNet*>(halfs[(i + 1) % N]);
            Endpoint* endpointA = halfToEndpoint(halfA);
            Endpoint* endpointB = halfToEndpoint(halfB);

            if (!endpointA || !endpointB) {
                failed = true;
                continue;
            }
            endpointA->mergeFaces(endpointB);
        }
    }

    if (failed) {
        // Handle error case
        std::cerr << "Do not know how this can happen, but halfToEndpoint is missing an endpoint." << std::endl;
        return &merged; // Return empty merged data
    }

    // Process edge data
    merged.edges.resize(edgeData.size());
    for (size_t i = 0; i < edgeData.size(); i++) {
        EdgeData& datum = edgeData[i];
        Line* line0 = datum.coreEndpoints[0]->getLine();

        for (size_t j = 1; j < datum.coreEndpoints.size(); j++) {
            Endpoint* endpointJ = datum.coreEndpoints[j];
            Line* lineJ = endpointJ->getLine();
            line0->addEndpoint(endpointJ, datum.halfEdges[j]->getEdgeIndex());
            lineJ->destroy();
        }

        // line0->fillFromEndpoints();
        merged.edges[i] = line0;

        if (datum.modified) {
            merged.vertices.push_back(datum.coreEndpoints[0]->getVertex());
            merged.vertices.push_back(datum.coreEndpoints[1]->getVertex());
        }
    }

    // Process end connectors
    //auto& endConnectors = endNet->getConnectors();
    //for (size_t i = 0; i < endConnectors.size(); i++) {
    //    auto halfs = endConnectors[i]->getInterior()->getHalfEdges();
    //    auto it = std::find_if(halfs.begin(), halfs.end(),
    //        [](HalfEdgeNet* half) { return half->getEdge() != nullptr; });

    //    if (it != halfs.end()) {
    //        auto faceHalfs = FaceNet::getConnectedHalfEdges(*it);
    //        HalfEdgeNet* endHalf = const_cast<HalfEdgeNet*>(faceHalfs.back());
    //        faceHalfs.pop_back();

    //        std::vector<Endpoint*> faceEndpointsI;
    //        std::transform(faceHalfs.begin(), faceHalfs.end(),
    //            std::back_inserter(faceEndpointsI),
    //            [this](HalfEdgeNet* half) { return halfToEndpoint(half); });

    //        TransistorPath* path = TransistorPath::createNet(faceEndpointsI, merged.edges, lines);
    //        Endpoint* pathEnd = halfToEndpoint(endHalf);
    //        std::vector<Endpoint*> pathEndpoints = { faceEndpointsI[0], pathEnd };
    //        path->setEndpoints(pathEndpoints);
    //        openPaths.push_back(path);
    //    }
    //}

    // Update connections
    /*std::set<Connection*> connectionsToUpdate;
    for (const auto& lineGroup : splitLines) {
        for (Line* line : lineGroup) {
            if (line) {
                auto endpoints = line->getEndpoints();
                // These are the endpoint's faceConnections.
                addConnectionsToSet(endpoints[0], connectionsToUpdate);
                addConnectionsToSet(endpoints[1], connectionsToUpdate);
                line->getNode()->destroy();
            }
        }
    }*/

    // *** TODO: Boxes is working as far as I can tell on everything except
    // the next code block for process faces.

    // Process faces
    /*merged.faces.clear();
    for (FaceNet* face : endNetwork->getFaces()) {
        HalfEdgeNet* half = face->getOuterComponent();
        if (half) {
            merged.faces.push_back(halfToEndpoint(half)->getFace());
        }
    }*/

    // TODO: Handle holes.
    // Process outer faces
    /*size_t faceIndex = 0;
    for (Face* face : endNet->getOuterFaces()) {
        merged.faces[faceIndex++]->setHole(true);
    }*/

    // TODO: Add Face groups back in.
    //for (Face* face : merged.faces) {
    //    if (!face->isHole() &&
    //        face->getGroup()->getFaces().indexOf(face) > 0) {
    //        face->splitGroup();
    //    }
    //}

    // Update outer faces map
    //map.outerFacesA.clear();
    //for (Face* endFace : endNet->getOuterFaces()) {
    //    map.outerFacesA.push_back(
    //        halfToEndpoint(endFace->getOuterComponent())->getFace());
    //}

    // Update connections and check success
    bool success = true;
    /*for (Connection* connection : connectionsToUpdate) {
        if (connection->getNode()->isDestroyed()) {
            continue;
        }
        auto endpoints = connection->getEndpoints();
        Face* face = connection->getFace();

        if (endpoints[1]) {
            success = success && face->updateConnection();
        }
    }*/

    Graph emptyGraph;
    return &(success ? merged : emptyGraph);
}

bool NetTransistor::solve() {
    setup();
    return sampleSolutionSpace();
}

void NetTransistor::setup() {
    // Set math library based on settings
    //if (GlobalSettings::get("Fast Matrix Math")) {
    //    mathG = &FastMath::getInstance();
    //}
    //else {
    //    mathG = &Math::getInstance();
    //}

    timer->start("Setup");
    setupFaceCentric();
    timer->stop("Setup");
}

// Static helper function
void NetTransistor::constrainVertexIds(std::vector<int>& vIds, NetTransistorSettings* settings) {
    while (!vIds.empty()) {
        std::vector<int> newVIdsToConstrain;
        VertexPlacement* mostConstrained = nullptr;
        int maxConstraints = -1;

        for (int id : vIds) {
            auto* vPlace = settings->getVertex(id);
            int numConstraints = vPlace->getNumConstraints();

            if (numConstraints < 3) {
                if (maxConstraints < numConstraints) {
                    maxConstraints = numConstraints;
                    mostConstrained = vPlace;
                }
                newVIdsToConstrain.push_back(id);
            }
        }

        if (mostConstrained) {
            mostConstrained->addConstraint();
        }

        vIds = std::move(newVIdsToConstrain);
    }
}

void NetTransistor::addFixedFace(Face* fixedFaceA, Face* fixedFaceB, double d) {
    //auto* fPlace = settings->getFace(fixedFaceB->getId());

    //// Check if face is already fixed
    //auto it = std::find_if(fixedFaces.begin(), fixedFaces.end(),
    //    [fPlace](const FixedFace& fixed) {
    //        return fixed.fPlace == fPlace;
    //    });

    //if (it != fixedFaces.end()) {
    //    return;
    //}

    //FixedFace fixedFace{ fixedFaceA, fPlace, d };
    //fixedFaces.push_back(fixedFace);
    //fPlace->makeFixed(fixedFace);
}

void NetTransistor::setupFaceCentric() {
    //auto settings = std::make_unique<NetTransistorSettings>();
    //this->settings = std::move(settings);

    //std::vector<int> basisIds;
    //std::vector<int> vertexIds;

    //// Process edges
    //for (auto* edge : graph->edges) {
    //    int id = edge->getNode()->getId();
    //    settings->edgePlacements[id] = std::make_unique<EdgePlacement>(edge, id, settings.get());
    //}

    //// Process vertices
    //for (auto* vertex : graph->vertices) {
    //    int id = vertex->getId();
    //    vertexIds.push_back(id);
    //    freeVertices.push_back(vertex);
    //    settings->vertexPlacements[id] = std::make_unique<VertexPlacement>(vertex, id, settings.get());
    //    settings->vertexPlacements[id]->initialize();
    //}

    //// Initialize edge placements
    //for (auto* edge : graph->edges) {
    //    int id = edge->getNode()->getId();
    //    settings->edgePlacements[id]->initialize();
    //}

    //// Check three faces for each vertex
    //for (int id : vertexIds) {
    //    settings->getVertex(id)->checkThreeFaces();
    //}

    //// Process fixed vertices from open paths
    //fixedVertexIds.clear();
    //for (auto* path : openPaths) {
    //    for (int j = 0; j < 2; ++j) {
    //        auto* pathVertex = path->endpoints[j]->getVertex();
    //        int id = pathVertex->getNode()->getId();
    //        if (std::find(fixedVertexIds.begin(), fixedVertexIds.end(), id) 
    //            == fixedVertexIds.end()) {
    //            fixedVertexIds.push_back(id);
    //        }
    //    }
    //}

    //// Process fixed faces
    //fixedFaces.clear();
    //auto& outerFaces = map->outerFaces ? map->outerFaces : std::vector<Face*>();
    //
    //for (size_t i = 0; i < outerFaces.size(); ++i) {
    //    auto* fixedFaceA = outerFaces[i];
    //    auto* endFace = endNet->getOuterFaces()[i];
    //    
    //    if (endFace) {
    //        int faceIndex = endNet->getInterior()->getFaceIndex(endFace);
    //        auto* fixedFaceB = graph->faces[faceIndex];
    //        double d;

    //        // Use the value from outerFacesA if fixedFaceA is destroyed.
    //        /*if (fixedFaceA->getNode()->isDestroyed()) {
    //            fixedFaceA = map->outerFacesA[i];
    //            d = map->outerFacesD[i];
    //        } else {*/
    //            auto normal = fixedFaceB->getFaceType()->getNormal();
    //            auto vPosition = fixedFaceA->getEndpoints()[0]->getVertex()->getPosition();
    //            d = normal.dot(vPosition);
    //        // }

    //        addFixedFace(fixedFaceA, fixedFaceB, d);
    //    }
    //}

    //// Process face placements
    //for (auto& [_, fPlace] : settings->facePlacements) {
    //    if (auto* face = fPlace->face) {
    //        auto* group = face->getGroup();
    //        if (group->getFaces().size() > 1) {
    //            auto normal = face->getFaceType()->getNormal();
    //            auto vPosition = face->getEndpoints()[0]->getVertex()->getPosition();
    //            double d = normal.dot(vPosition);
    //            addFixedFace(face, face, d);
    //        }
    //    }
    //}
}

Limits NetTransistor::findLimits() {
    std::vector<double> minLimit;
    std::vector<double> maxLimit;

    // Handle vertex limits
    /*for (size_t i = 0; i < freeVertices.size(); i++) {
        for (int dim = 0; dim < dims; dim++) {
            minLimit.push_back(mutationArea.lowerExtent[dim]);
            maxLimit.push_back(mutationArea.upperExtent[dim]);
        }
    }*/

    // Handle edge limits
    //for (auto* edge : freeEdges) {
    //    auto* edgeType = edge->getEdgeType();
    //    auto* brush = edgeType->getBrush();

    //    double minLength = minLength;
    //    double maxLength = std::numeric_limits<double>::infinity();

    //    if (brush && brush->get("Strict Length")) {
    //        minLength = brush->get("Min Length");
    //        maxLength = brush->get("Max Length");
    //    }

    //    minLimit.push_back(minLength);
    //    maxLimit.push_back(maxLength);
    //}

    return { minLimit, maxLimit };
}

bool NetTransistor::hasViolations(const std::vector<double>& positions, const Limits& limits) {
    for (size_t i = 0; i < positions.size(); i++) {
        double value = positions[i];
        if (value < limits.min[i] || value > limits.max[i]) {
            return true;
        }
    }
    return false;
}

//Range NetTransistor::getRange(const std::vector<int>& orderIds,
//    const std::vector<OrderInfo>& orderInfo) {
//    Range range(-std::numeric_limits<double>::infinity(),
//        std::numeric_limits<double>::infinity());
//
//    for (size_t i = 0; i < orderIds.size(); i++) {
//        int id = orderIds[i];
//        const auto& info = orderInfo[i];
//        Range rangeI;
//
//        if (info.type == "vertex") {
//            rangeI = settings->getVertex(id)->getRange();
//        }
//        else if (info.type == "edge") {
//            rangeI = settings->getEdge(id)->getRange();
//        }
//        else if (info.type == "face") {
//            rangeI = settings->getFace(id)->getRange(info.vertexId);
//        }
//
//        range = range.intersect(rangeI);
//    }
//
//    return range;
//}

//void NetTransistor::setPlacements(const std::vector<int>& orderIds,
//    const std::vector<OrderInfo>& orderInfo) {
//    for (size_t i = 0; i < orderIds.size(); i++) {
//        int id = orderIds[i];
//        const auto& info = orderInfo[i];
//
//        if (info.type == "vertex") {
//            settings->getVertex(id)->setPosition();
//        }
//        else if (info.type == "face") {
//            settings->getFace(id)->setFromVertex(info.vertexId);
//        }
//    }
//}

std::vector<double> NetTransistor::sampleFaceCentric() {
//    if (ground) {
//        std::vector<double> result;
//        auto& vertices = endNet->getInterior()->getVertices();
//
//        if (vertices.size() == 4) {
//            for (auto* vertex : vertices) {
//                if (dims == 3) {
//                    auto dir = vertex->getHalfEdges()[0]->getDir();
//                    if (dir.x > 0.9) {          // +X
//                        result.insert(result.end(), { lower[0], lower[1], lower[2] + 1 });
//                    }
//                    else if (dir.x < -0.9) {  // -X
//                        result.insert(result.end(), { upper[0], upper[1], lower[2] + 1 });
//                    }
//                    else if (dir.y > 0.9) {   // +Y
//                        result.insert(result.end(), { upper[0], lower[1], lower[2] + 1 });
//                    }
//                    else if (dir.y < -0.9) {  // -Y
//                        result.insert(result.end(), { lower[0], upper[1], lower[2] + 1 });
//                    }
//                }
//                else {
//                    int signX = 0, signY = 0;
//                    for (int i = 0; i < 2; i++) {
//                        auto dir = vertex->getHalfEdges()[i]->getDir();
//                        if (dir.x > 0.9) signX = 1;
//                        if (dir.x < -0.9) signX = -1;
//                        if (dir.y > 0.9) signY = 1;
//                        if (dir.y < -0.9) signY = -1;
//                    }
//
//                    if (signX == 0 || signY == 0) {
//                        Alert::show("2D boundary corner is wrong.");
//                    }
//
//                    double xPos = (signX == 1) ? lower[0] + 0.1 : upper[0] - 0.1;
//                    double yPos = (signY == 1) ? lower[1] + 0.1 : upper[1] - 0.1;
//                    result.insert(result.end(), { xPos, yPos });
//                }
//            }
//            return result;
//        }
//    }
//
//    bool success = true;
//    for (const auto& fixed : fixedFaces) {
//        fixed.fPlace->setD(fixed.d);
//        fixed.fPlace->setFixed(true);
//    }
//
//    for (int id : fixedVertexIds) {
//        success = success && settings->getVertex(id)->fixPosition();
//    }
//
//    if (!success) return {};
//
//    std::vector<int> basisOrders;
//    for (size_t i = 0; i < settings->basisIds.size(); i++) {
//        auto basisOrder = std::find_if(settings->orderIds.begin(),
//            settings->orderIds.end(),
//            [&](const auto& id) {
//                return id == settings->basisIds[i] &&
//                    settings->orderInfo[i].type == "face";
//            }) - settings->orderIds.begin();
//            basisOrders.push_back(basisOrder);
//    }
//
//    for (size_t i = 0; i < settings->basisIds.size(); i++) {
//        int id = settings->basisIds[i];
//        auto* fPlace = settings->facePlacements[id].get();
//        int start = basisOrders[i];
//        int end = basisOrders[i + 1];
//
//        std::vector<int> orderIds(settings->orderIds.begin() + start,
//            settings->orderIds.begin() + end);
//        std::vector<OrderInfo> orderInfo(settings->orderInfo.begin() + start,
//            settings->orderInfo.begin() + end);
//
//        auto range = getRange(orderIds, orderInfo);
//
//        if (fPlace->getFixed() && !range.isInside(fPlace->getD())) {
//            effort = std::numeric_limits<double>::infinity();
//            return {};
//        }
//
//        if (range.isEmpty()) {
//            return {};
//        }
//
//        if (!fPlace->getFixed()) {
//            double d = range.sample();
//            fPlace->setD(d);
//        }
//
//        orderIds.erase(orderIds.begin());
//        orderInfo.erase(orderInfo.begin());
//        setPlacements(orderIds, orderInfo);
//    }
//
    std::vector<double> positions;
//    for (auto* vertex : freeVertices) {
//        int id = vertex->getNode()->getId();
//        auto position = settings->vertexPlacements[id]->getPosition();
//        for (int j = 0; j < dims; j++) {
//            positions.push_back(position.getValue(j));
//        }
//    }
//
    return positions;
}

bool NetTransistor::sampleSolutionSpace() {
    timer->start("Sample Solutions");

    effort = 0;
    auto limits = findLimits();

    while (true) {
        if (effort > maxEffort) {
            timer->stop("Sample Solutions");
            return false;
        }

        auto positions = sampleFaceCentric();
        bool violated = positions.empty() || hasViolations(positions, limits);

        if (!violated) {
            timer->stop("Sample Solutions");
            timer->start("Place Vertices");

            if (placeVertexPositions(positions)) {
                timer->stop("Place Vertices");
                return true;
            }

            timer->stop("Place Vertices");
            timer->start("Sample Solutions");
        }
        effort++;
    }
}

std::vector<TransistorPath*> NetTransistor::getFreeablePaths() const {
    std::vector<TransistorPath*> result;
    std::copy_if(openPaths.begin(), openPaths.end(), std::back_inserter(result),
        [](TransistorPath* path) {
            return path->endpoints[0] != path->endpoints[1] &&
                path->extendableness() > 0;
        });
    return result;
}

void NetTransistor::freeVertex() {
    auto freeablePaths = getFreeablePaths();
    if (freeablePaths.empty()) {
        effort++;
        return;
    }

    // Pick a random path
    auto* path = Util::pick(freeablePaths);
    auto* vertex = path->randomNextVertex();
    freeOneVertex(vertex);

    bool done = false;
    while (!done) {
        done = true;
        freeablePaths = getFreeablePaths();

        for (auto* path : freeablePaths) {
            if (auto* rigidVertex = path->rigidNextVertex()) {
                freeOneVertex(rigidVertex);
                done = false;
                break;
            }
        }
    }
}

void NetTransistor::freeOneVertex(Vertex* vertex) {
    //auto extents = model->getExtents();
    //auto vertexEndpoints = vertex->getEndpoints();

    //// Process all vertex endpoints
    //for (auto* vEndpoint : vertexEndpoints) {
    //    auto* line = vEndpoint->getLine();
    //    auto hasLine = [line](const LineData& data) { return data.line == line; };
    //    
    //    if (std::find_if(lineData.begin(), lineData.end(), hasLine) == lineData.end()) {
    //        addLine(line, true, true);
    //        auto* lineState = line->getSegment()->getStates()[0];
    //        lineState->removeCells();
    //    }
    //}

    //// Process paths for each endpoint
    //for (auto* vEndpoint : vertexEndpoints) {
    //    auto* line = vEndpoint->getLine();
    //    
    //    auto findPath0 = [vEndpoint](TransistorPath* path) { 
    //        return path->endpoints[0] == vEndpoint; 
    //    };
    //    auto findPath1 = [vEndpoint](TransistorPath* path) { 
    //        return path->endpoints[1] == vEndpoint; 
    //    };

    //    auto* path0 = std::find_if(openPaths.begin(), openPaths.end(), findPath0);
    //    auto* path1 = std::find_if(openPaths.begin(), openPaths.end(), findPath1);

    //    if (path0 != openPaths.end() && path1 != openPaths.end()) {
    //        if (*path0 == *path1) {
    //            // Same path
    //            (*path0)->endpoints.clear();
    //            Remove::fromVector(openPaths, *path0);
    //        } else {
    //            // Different paths
    //            (*path1)->merge(*path0);
    //            Remove::fromVector(openPaths, *path0);
    //        }
    //    } else if (path0 != openPaths.end()) {
    //        (*path0)->expandBackward();
    //    } else if (path1 != openPaths.end()) {
    //        (*path1)->expandForward();
    //    } else {
    //        // Create new path
    //        auto* path = new TransistorPath({}, lines);
    //        path->setEndpoints({vEndpoint, vEndpoint});
    //        path->expandBackward();
    //        path->expandForward();
    //        openPaths.push_back(path);
    //    }
    //}
}

bool NetTransistor::placeVertexPositions(const std::vector<double>& positions) {
    //// Place vertices at their new positions
    //for (size_t i = 0; i < freeVertices.size(); i++) {
    //    Vec3 position;
    //    if (dims == 2) {
    //        position = Vec3(positions[dims * i], 
    //                      positions[dims * i + 1], 
    //                      0.0);
    //    } else {
    //        position = Vec3(positions[dims * i], 
    //                      positions[dims * i + 1], 
    //                      positions[dims * i + 2]);
    //    }
    //    
    //    freeVertices[i]->setPosition(position);
    //    if (!model->inBounds(position.x, position.y, position.z)) {
    //        return false;
    //    }
    //}

    //// Faces to update the face connection
    //std::set<Face*> facesToUpdate;

    //// 2D Edge Intersections
    //if (dims == 2) {
    //    for (size_t i = 0; i < propagationOrder.size(); i++) {
    //        auto* endpoint = propagationOrder[i];
    //        auto* line = endpoint->getLine();
    //        line->moveToEndpoints();
    //        auto* lineState = line->getSegment()->getStates()[0];
    //        
    //        auto intersection = lineState->addToModelWithIntersections(-std::numeric_limits<double>::infinity());
    //        if (intersection) {
    //            if (!intersection->state) {
    //                return false;
    //            }
    //            // Reset the line states, remove them from the model
    //            for (size_t j = 0; j <= i; j++) {
    //                auto* endpoint = propagationOrder[j];
    //                auto* line = endpoint->getLine();
    //                auto* lineState = line->getSegment()->getStates()[0];
    //                lineState->removeCells();
    //            }
    //            return false;
    //        }

    //        auto intersections = lineState->findConnectionIntersections();
    //        for (const auto& connection : intersections) {
    //            facesToUpdate.insert(connection->getFace());
    //        }
    //    }
    //}

    //// Collect core vertices
    //std::vector<Vertex*> coreVertices;
    //for (auto* path : openPaths) {
    //    coreVertices.push_back(path->endpoints[0]->getVertex());
    //}
    //coreVertices.insert(coreVertices.end(), freeVertices.begin(), freeVertices.end());

    ////if (dims == 2) {
    ////    // Find all faces that were involved
    ////    auto addFaces = [&facesToUpdate](Endpoint* endpoint) {
    ////        facesToUpdate.insert(endpoint->getFace());
    ////        for (auto* connection : endpoint->getConnections()) {
    ////            facesToUpdate.insert(connection->getFace());
    ////        }
    ////    };

    ////    for (auto* edge : freeEdges) {
    ////        auto& endpoints = edge->getEndpoints();
    ////        addFaces(endpoints[0]);
    ////        addFaces(endpoints[1]);
    ////    }

    ////    // Update face connections
    ////    bool success = true;
    ////    for (auto* face : facesToUpdate) {
    ////        success = success && face->updateConnection();
    ////    }
    ////    if (!success) {
    ////        return false;
    ////    }

    ////    // Handle tiled edges
    ////    for (auto* edge : freeEdges) {
    ////        auto* brush = edge->getBrush();
    ////        bool tiled = brush ? brush->get("Tiled") : true;
    ////        if (tiled && brush) {
    ////            auto& endpoints = edge->getEndpoints();
    ////            double length = endpoints[0]->getPosition().distance(endpoints[1]->getPosition());
    ////            double lengthInTiles = length / brush->get("Tile Length");
    ////            int numTiles = std::max(1, static_cast<int>(std::round(lengthInTiles)));
    ////            edge->getState()->getCoordinates()->angledEdges[1].t = numTiles;
    ////        }
    ////    }
    ////}
    //// 3D case
    //std::vector<Face*> facesToIntersect;
    //for (const auto& [_, fPlace] : settings->facePlacements) {
    //    if (fPlace->face) {
    //        facesToIntersect.push_back(fPlace->face);
    //    }
    //}

    //// Check face containment
    //bool success = std::all_of(facesToIntersect.begin(), facesToIntersect.end(),
    //    [this](Face* faceA) {
    //        auto faces = faceA->getGroup()->getFaces();
    //        if (faces.size() > 1) {
    //            if (faceA->isHole()) {
    //                if (!faces[0]->containsFace(faceA)) {
    //                    return false;
    //                }
    //                if (GlobalSettings::get("Cut Holes")) {
    //                    return std::all_of(faces.begin(), faces.end(),
    //                        [faceA](Face* faceB) {
    //                            return faceA == faceB || faceB == faces[0] || 
    //                                    faceA->outsideFace(faceB);
    //                        });
    //                }
    //                return true;
    //            } else {
    //                return std::all_of(faces.begin(), faces.end(),
    //                    [faceA](Face* faceB) {
    //                        return faceA == faceB || faceA->containsFace(faceB);
    //                    });
    //            }
    //        }
    //        return true;
    //    });

    //if (!success) {
    //    return false;
    //}

    //// Handle BSP tree operations
    //BspTree* tree = model->getBspTree();
    //for (size_t i = 0; i < facesToIntersect.size(); i++) {
    //    auto* face = facesToIntersect[i];
    //    auto* plane = BspPlane::create(face);
    //    auto* polygon = BspPolygon::create(face);

    //    bool success = ground || !polygon->selfIntersects();
    //    success = success && tree->add(plane, polygon);

    //    if (!success) {
    //        for (size_t j = 0; j <= i; j++) {
    //            auto* face = facesToIntersect[j];
    //            while (!face->getPolygons().empty()) {
    //                face->getPolygons()[0]->getNode()->destroy();
    //            }
    //        }
    //        return false;
    //    }
    //}

    return true;
}

void NetTransistor::reject() {
    // Clean up resources
    delete graph;
    graph = nullptr;
    
    // Clear vectors
    lineData.clear();
    lines.clear();
    freeVertices.clear();
    freeEdges.clear();
    // edgeBlockers.clear();
    propagationOrder.clear();
    basisEdges.clear();
    
    // Reset pointers and values
    // changeBasis = nullptr;
    initialPosition = nullptr;
    effort = std::numeric_limits<double>::infinity();
}

} // namespace ms 