#include "pch.h"
#include "rule_applier.h"
#include "../graph/graph.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/face.h"
#include "../settings.h"
#include "../util/timer.h"
#include "../util/util.h"
#include <iostream>
#include <set>
#include <utility>

unique_ptr<RuleApplier> RuleApplier::buildNormally(
    const Production& production, Model* model, int dims) {
    
    auto result = make_unique<RuleApplier>();
    result->create(production, model, dims);
    
    if (result->effort > 0) {
        result->reject();
        return nullptr;
    }
    return result;
}

void RuleApplier::create(const Production& production, Model* model_, int dims_) {
    startGraph = production.startGraph;
    model = model_;
    endGraph = production.endGraph;
 
    morphism = production.morphism;
    ground = production.ground;
    dims = dims_;

    effort = 0;

    timer->start("Create Graph");
    // TODO: Merge duplicate edges
    /*if (!mergeDuplicateEdges()) {
        effort = numeric_limits<double>::infinity();
        return;
    }*/

    graph = createGraph();
    timer->stop("Create Graph");

    if (!graph) {
        effort = numeric_limits<double>::infinity();
        return;
    }

    for (auto* edge : graph->edges) {
        addEdge(edge, false);
    }
}

void RuleApplier::addEdge(Edge* edge, bool addToGraph) {
    edges.push_back(edge);

    if (addToGraph) {
        auto edgeHalfEdges = edge->getHalfEdges();
        for (auto* halfEdge : edgeHalfEdges) {
            Util::union_(graph->vertices, {halfEdge->getVertex()});
        }
        graph->edges.push_back(edge);
    }
}

static void destroyEdges(const vector<vector<Edge*>>& edgeGroup) {
    set<HalfEdge*> halfEdgeSet;
    set<Vertex*> vertexSet;
    for (const auto& edgeGroup : edgeGroup) {
        for (Edge* edge : edgeGroup) {
            if (edge) {
                auto halfEdges = edge->getHalfEdges();
                halfEdgeSet.insert(halfEdges[0]);
                halfEdgeSet.insert(halfEdges[1]);
                edge->destroy();
            }
        }
    }
    for (const auto& halfEdge : halfEdgeSet) {
        if (halfEdge) {
            halfEdge->getFace()->removeHalfEdge(halfEdge);
            vertexSet.insert(halfEdge->getVertex());
            halfEdge->destroy();
        }
    }
    for (const auto& vertex : vertexSet) {
        if (vertex) {
            vertex->destroy();
        }
    }
}

EditGraph* RuleApplier::createGraph() {
    auto& endVertices = endGraph->getVertices();
    auto& endEdges = endGraph->getEdges();
    EditGraph* merged = new EditGraph();
    
    // Save face locations
    /*if (morphism->outerFaces) {
        morphism->outerFacesD.clear();
        for (auto* outerFace : morphism->outerFaces) {
            morphism->outerFacesD.push_back(
                outerFace->getFaceType()->getNormal().dot(
                    outerFace->getHalfEdges()[0]->getPosition()));
        }
    }*/

    // Maps from half edges to merge halfEdges
    vector<HalfEdge*> mergedHalfEdges(endGraph->getHalfEdges().size());
    fill(mergedHalfEdges.begin(), mergedHalfEdges.end(), nullptr);
    auto halfToHalfEdge = [&](GraphHalfEdge* half) -> HalfEdge* {
        int index = Util::findIndex<GraphHalfEdge*>(endGraph->getHalfEdges(), half);
        return mergedHalfEdges[index];
    };
    
    auto setHalfToHalfEdge = [&](GraphHalfEdge* half, HalfEdge* halfEdge) {
        int index = Util::findIndex<GraphHalfEdge*>(endGraph->getHalfEdges(), half);
        mergedHalfEdges[index] = halfEdge;
    };

    vector<vector<Edge*>> splitEdges;
    vector<vector<HalfEdge*>> splitHalfEdges;
    if (morphism) {
        splitEdges.resize(morphism->edgeBtoA.size());
        splitHalfEdges.resize(morphism->edgeBtoA.size());
        for (size_t i = 0; i < morphism->edgeBtoA.size(); ++i) {
            splitEdges[i] = {morphism->edgeBtoA[i]};
        }
    }

    // Create vertices
    const auto numVertices = endVertices.size();
    merged->vertices.resize(numVertices, nullptr);
    for (size_t i = 0; i < numVertices; ++i) {
        auto* v = endVertices[i];

        if (v->boundaryIndex() < 0) {
            auto* type = v->getType();
            
            // Create a random position. This was for debugging in the web version.
            // The position is later replaced.
            Vec3 randomPosition = Vec3(
                5.0 * random(),
                5.0 * random(),
                5.0 * random()
            );

            auto* newVertex = new Vertex(model, randomPosition, type);
            newVertex->createHalfEdges();
            merged->vertices[i] = newVertex;
            
            auto vHalfEdges = newVertex->getHalfEdges();
            auto& halfs = v->getHalfEdges();
            
            for (size_t j = 0; j < halfs.size(); ++j) {
                setHalfToHalfEdge(halfs[j], vHalfEdges[j]);
            }
        }
    }
    if (startGraph->getId() == 0) {
        int x = 0;
    }

    struct EdgeData {
        vector<HalfEdge*> coreHalfEdges;
        vector<GraphHalfEdge*> halfEdges;
        bool modified;
    };

    // Process edges
    vector<EdgeData> edgeData;
    for (size_t i = 0; i < endEdges.size(); ++i) {
        auto* endEdge = endEdges[i];
        auto& edgeHalfs = endEdge->getHalfEdges();
        vector<HalfEdge*> coreHalfEdges;
        vector<GraphHalfEdge*> halfEdges;
        bool modified = false;

        for (size_t e = 0; e < edgeHalfs.size(); ++e) {
            auto* half = edgeHalfs[e][0];
            auto* hVertex = half->getVertex();
            int hIndex = Util::findIndex(endVertices, hVertex);
            auto* core = merged->vertices[hIndex];

            int boundaryIndex = hVertex->boundaryIndex();
            if (boundaryIndex >= 0) {
                int startIndex = Util::findIndex(startGraph->getEdges(),
                    startGraph->getBVertices()[boundaryIndex]->interiorEdge());
                
                if (splitEdges[startIndex].size() == 1) {
                    int count = 0;
                    for (int i = 0; i < splitEdges.size(); i++) {
                        if (splitEdges[i][0] == splitEdges[startIndex][0]) {
                            count++;
                            if (count >= 2) {
                                // The same edge is being split twice. This should not happen. Exit early.
                                return nullptr;
                            }
                        }
                    }
                    auto split = splitEdges[startIndex][0]->split(true);
                    splitEdges[startIndex] = split.edges;
                    splitHalfEdges[startIndex] = split.nextHalfEdges;
                }

                int edgeIndex = half->getForward() ? 0 : 1;
                auto* coreHalfEdge = splitEdges[startIndex][edgeIndex]->getHalfEdges()[e];
                splitEdges[startIndex][edgeIndex] = nullptr;
                coreHalfEdges.push_back(coreHalfEdge);

                halfEdges.push_back(half);
                modified = true;
                setHalfToHalfEdge(half, coreHalfEdge);
            } else {
                auto* coreHalfEdge = core->getHalfEdges()[half->getVertexIndex()];
                coreHalfEdges.push_back(coreHalfEdge);
                halfEdges.push_back(half);
            }
        }
        
        edgeData.push_back({coreHalfEdges, halfEdges, modified});
    }

    // Process end edges.
    for (size_t i = 0; i < endEdges.size(); i++) {
        GraphEdge* endEdge = endEdges[i];
        auto edgeHalfs = endEdge->getHalfEdges();

        for (size_t e = 0; e < edgeHalfs.size(); e++) {
            GraphHalfEdge* halfNext = edgeHalfs[e][0]->getNext();
            if (!halfNext->getEdge()) {
                int boundaryIndex = indexOf(endGraph->getBHalfEdges(), halfNext);
                auto startHalf = startGraph->getBHalfEdges()[boundaryIndex];
                int edgeIndex = indexOf(startGraph->getEdges(), startHalf->getPrev()->getEdge());
                int startIndex = indexOf(startGraph->getHalfEdges(), startHalf);
                setHalfToHalfEdge(halfNext, splitHalfEdges[edgeIndex][e]);                
            }
        }
    }

    bool failed = false;

    // Process faces in end graph.
    for (GraphFace* face : endGraph->getFaces()) {
        auto halfs = face->getOuterHalfEdges();
        size_t N = halfs.size();

        for (size_t i = 0; i < N; i++) {
            GraphHalfEdge* halfA = const_cast<GraphHalfEdge*>(halfs[i]);
            GraphHalfEdge* halfB = const_cast<GraphHalfEdge*>(halfs[(i + 1) % N]);
            HalfEdge* halfEdgeA = halfToHalfEdge(halfA);
            HalfEdge* halfEdgeB = halfToHalfEdge(halfB);

            if (!halfEdgeA || !halfEdgeB) {
                failed = true;
                continue;
            }
            halfEdgeA->mergeFaces(halfEdgeB);
        }
    }

    if (failed) {
        // Handle error case
        cerr << "Do not know how this can happen, but halfToHalfEdge is missing an halfEdge." << endl;
        return merged; // Return empty merged data
    }

    // Process edge data.
    merged->edges.resize(edgeData.size());
    for (size_t i = 0; i < edgeData.size(); i++) {
        EdgeData& datum = edgeData[i];
        Edge* edge0 = datum.coreHalfEdges[0]->getEdge();

        for (size_t j = 1; j < datum.coreHalfEdges.size(); j++) {
            HalfEdge* halfEdgeJ = datum.coreHalfEdges[j];
            Edge* edgeJ = halfEdgeJ->getEdge();
            edge0->addHalfEdge(halfEdgeJ, datum.halfEdges[j]->getEdgeIndex());
            edgeJ->destroy();
        }
        merged->edges[i] = edge0;

        if (datum.modified) {
            vector<Vertex*> newVertices = {
                datum.coreHalfEdges[0]->getVertex(),
                datum.coreHalfEdges[1]->getVertex()
            };
            Util::union_(merged->vertices, newVertices);
        }
    }

    // Process boundary vertices.
    auto& bVertices = endGraph->getBVertices();
    for (size_t i = 0; i < bVertices.size(); i++) {
        auto halfs = bVertices[i]->getHalfEdges();
        auto it = find_if(halfs.begin(), halfs.end(),
            [](GraphHalfEdge* half) { return half->getEdge() != nullptr; });

        if (it != halfs.end()) {
            auto faceHalfs = GraphFace::getConnectedHalfEdges(*it);
            GraphHalfEdge* endHalf = const_cast<GraphHalfEdge*>(faceHalfs.back());
            faceHalfs.pop_back();

            vector<HalfEdge*> faceHalfEdgesI;
            for (GraphHalfEdge* half : faceHalfs) {
                faceHalfEdgesI.push_back(halfToHalfEdge(half));
            }

            MorphismPath* path = MorphismPath::createPath(faceHalfEdgesI, merged->edges, &edges);
            HalfEdge* pathEnd = halfToHalfEdge(endHalf);
            vector<HalfEdge*> pathHalfEdges = { faceHalfEdgesI[0], pathEnd };
            path->setHalfEdges(pathHalfEdges);
            openPaths.push_back(path);
        }
    }

    destroyEdges(splitEdges);

    // Process faces.
    merged->faces.clear();
    for (GraphFace* face : endGraph->getFaces()) {
        GraphHalfEdge* half = face->getOuterComponent();
        if (half) {
            merged->faces.push_back(halfToHalfEdge(half)->getFace());
        }
    }

    // Process outer faces.
    size_t faceIndex = 0;
    for (GraphFace* outerFace : endGraph->getBFaces()) {
        faceIndex = indexOf(endGraph->getFaces(), outerFace);
        merged->faces[faceIndex]->setHole(true);
    }
    for (Face* face : merged->faces) {
		// Split the face off from a group if it is not a hole, but is not in
		// outer position. It was split from another face, but is no longer coplanar.
        if (!face->isHole() &&
            indexOf(face->getGroup()->getFaces(), face) > 0) {
            face->splitGroup();
        }
    }

    // Update outer faces morphism
    //morphism.outerFacesA.clear();
    //for (Face* endFace : endGraph->getOuterFaces()) {
    //    morphism.outerFacesA.push_back(
    //        halfToHalfEdge(endFace->getOuterComponent())->getFace());
    //}

    return merged;
}

bool RuleApplier::solve() {
    setup();
    return sampleSolutionSpace();
}

void RuleApplier::setup() {
    timer->start("Setup");
    setupFaceCentric();
    timer->stop("Setup");
}

// Static helper function.
void RuleApplier::constrainVertexIds(vector<int>& vertexIds, RuleApplierSettings* settings) {
    vector vIds(vertexIds);
    while (!vIds.empty()) {
        vector<int> newVIdsToConstrain;
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

        vIds = move(newVIdsToConstrain);
    }
}

// Connect a new face as an inner component or hole to this one.
void RuleApplier::addFixedFace(Face* fixedFaceA, Face* fixedFaceB, double d) {
    auto* fPlace = settings->getFace(fixedFaceB->getId());

    // Check if face is already fixed
    auto it = find_if(fixedFaces.begin(), fixedFaces.end(),
        [fPlace](const FixedFace& fixed) {
            return fixed.fPlace == fPlace;
        });

    if (it != fixedFaces.end()) {
        return;
    }

    FixedFace fixedFace{ fixedFaceA, fPlace, d };
    fixedFaces.push_back(fixedFace);
    fPlace->makeFixed(fixedFace);
}

vector<double> RuleApplier::getExtents() {
    auto extents = globalSettings["Extents"].get<vector<double>>();
    if (dims == 2) {
        extents[2] = 0;
    }
    return extents;
}

void RuleApplier::setupFaceCentric() {
    auto extents = getExtents();
    const Vec3 lower(1, 1, 0);
    const Vec3 upper(extents[0] - 1, extents[1] - 1, extents[2]);
    auto settingsNew = make_unique<RuleApplierSettings>(lower, upper);
    this->settings = move(settingsNew);
    freeVertices.clear();

    vector<int> basisIds;
    vector<int> vertexIds;

    // Process edges
    for (auto* edge : graph->edges) {
        int id = edge->getId();
        settings->edgePlacements[id] = make_unique<EdgePlacement>(edge, id, settings.get());
    }

    // Process vertices
    for (auto* vertex : graph->vertices) {
        if (!vertex) {
            continue;
        }
        int id = vertex->getId();
        vertexIds.push_back(id);
        freeVertices.push_back(vertex);
        settings->vertexPlacements[id] = make_unique<VertexPlacement>(vertex, id, settings.get());
        settings->vertexPlacements[id]->initialize();
    }

    // Initialize edge placements
    for (auto* edge : graph->edges) {
        int id = edge->getId();
        settings->edgePlacements[id]->initialize();
    }

    // Check three faces for each vertex
    for (int id : vertexIds) {
        settings->getVertex(id)->checkThreeFaces();
    }

    // Process fixed vertices from open paths.
    fixedVertexIds.clear();
    for (auto* path : openPaths) {
        for (int j = 0; j < 2; ++j) {
            auto* pathVertex = path->halfEdges[j]->getVertex();
            int id = pathVertex->getId();
            if (!contains(fixedVertexIds, id)) {
                fixedVertexIds.push_back(id);
            }
        }
    }

    // Process fixed faces
    fixedFaces.clear();
    auto& faceBtoA = morphism->faceBtoA;
    
    for (size_t i = 0; i < faceBtoA.size(); ++i) {
        auto* fixedFaceA = faceBtoA[i];
        auto* endFace = endGraph->getBFaces()[i];
        
         if (endFace) {
             int faceIndex = indexOf(endGraph->getFaces(), endFace);
             auto* fixedFaceB = graph->faces[faceIndex];
             double d;

             // Use the value from outerFacesA if fixedFaceA is destroyed.
             /*if (fixedFaceA->getNode()->isDestroyed()) {
                 fixedFaceA = morphism->outerFacesA[i];
                 d = morphism->outerFacesD[i];
             } else {*/
                 auto normal = fixedFaceB->getFaceType()->getNormal();
                 auto vPosition = fixedFaceA->getHalfEdges()[0]->getVertex()->getPosition();
                 d = normal.dot(vPosition);
             // }

             addFixedFace(fixedFaceA, fixedFaceB, d);
         }
    }

    // Process face placements
    for (auto& [_, fPlace] : settings->facePlacements) {
        auto face = fPlace->getFace();
        if (face) {
            auto* group = face->getGroup();
            if (group->getFaces().size() > 1) {
                auto normal = face->getFaceType()->getNormal();
                auto vPosition = face->getHalfEdges()[0]->getVertex()->getPosition();
                double d = normal.dot(vPosition);
                addFixedFace(face, face, d);
            }
        }
    }

    constrainVertexIds(fixedVertexIds, settings.get());
    constrainVertexIds(vertexIds, settings.get());
}

Limits RuleApplier::findLimits() {
    vector<double> minLimit;
    vector<double> maxLimit;
    auto extents = getExtents();

    // Handle vertex limits
    for (size_t i = 0; i < freeVertices.size(); i++) {
        for (int dim = 0; dim < 3; dim++) {
            minLimit.push_back(0);
            maxLimit.push_back(extents[dim]);
        }
    }

    // Handle edge limits
    for (auto* edge : freeEdges) {

        double minLength = 0;
        double maxLength = numeric_limits<double>::infinity();

        // TODO: Use edgeSettings to set edge lengths.
        /*auto* edgeType = edge->getEdgeType();
        auto* edgeSettings = edgeType->getEdgeSettings();
        if (edgeSettings && edgeSettings->get("Strict Length")) {
            minLength = edgeSettings->get("Min Length");
            maxLength = edgeSettings->get("Max Length");
        }*/

        minLimit.push_back(minLength);
        maxLimit.push_back(maxLength);
    }

    return { minLimit, maxLimit };
}

bool RuleApplier::hasViolations(const vector<double>& positions, const Limits& limits) {
    for (size_t i = 0; i < positions.size(); i++) {
        double value = positions[i];
        if (value < limits.min[i] || value > limits.max[i]) {
            return true;
        }
    }
    return false;
}

Range RuleApplier::getRange(
    const vector<int>& orderIds,
    const vector<OrderInfo>& orderInfo
) {
    Range range(-numeric_limits<double>::infinity(),
        numeric_limits<double>::infinity());

    for (size_t i = 0; i < orderIds.size(); i++) {
        int id = orderIds[i];
        const auto& info = orderInfo[i];

        Range rangeI;
        if (info.type == "vertex") {
            rangeI = settings->getVertex(id)->getRange();
        } else if (info.type == "edge") {
            rangeI = settings->getEdge(id)->getRange();
        } else if (info.type == "face") {
            rangeI = settings->getFace(id)->getRange(info.vertexId);
        }
        range = range.intersect(rangeI);
    }

    return range;
}

void RuleApplier::setPlacements(
    const vector<int>& orderIds,
    const vector<OrderInfo>& orderInfo
) {
    for (size_t i = 0; i < orderIds.size(); i++) {
        int id = orderIds[i];
        const auto& info = orderInfo[i];

        if (info.type == "vertex") {
            settings->getVertex(id)->setPosition();
        } else if (info.type == "face") {
            settings->getFace(id)->setFromVertex(info.vertexId);
        }
    }
}

pair<vector<double>, bool> RuleApplier::sampleFaceCentric() {
    if (ground) {
        vector<double> result;
        auto& vertices = endGraph->getVertices();

        if (vertices.size() == 4) {
            vector<double> lower = {0, 0, 0};
            auto upper = getExtents();
            for (auto* vertex : vertices) {
                auto dir = vertex->getHalfEdges()[0]->getDir();
                if (dir.getX() > 0.9) {          // +X
                    result.insert(result.end(), { lower[0], lower[1], lower[2] + 1 });
                } else if (dir.getX() < -0.9) {  // -X
                    result.insert(result.end(), { upper[0], upper[1], lower[2] + 1 });
                } else if (dir.getY() > 0.9) {   // +Y
                    result.insert(result.end(), { upper[0], lower[1], lower[2] + 1 });
                } else if (dir.getY() < -0.9) {  // -Y
                    result.insert(result.end(), { lower[0], upper[1], lower[2] + 1 });
                }
            }
            return make_pair(result, true);
        }
    }

    bool success = true;
    for (const auto& fixed : fixedFaces) {
        fixed.fPlace->setD(fixed.d);
        fixed.fPlace->setFixed(true);
    }

    for (int id : fixedVertexIds) {
        success = success && settings->getVertex(id)->fixPosition();
    }

    if (!success) {
        return {};
    }

    vector<int> basisOrders;
    for (const auto& basisId : settings->basisIds) {
        basisOrders.push_back(settings->findBasisOrder(basisId));
    }
    basisOrders.push_back((int)settings->orderIds.size());

    for (size_t i = 0; i < settings->basisIds.size(); i++) {
        int id = settings->basisIds[i];
        auto* fPlace = settings->facePlacements[id].get();
        int start = basisOrders[i];
        int end = basisOrders[i + 1];

        vector<int> orderIds(settings->orderIds.begin() + start,
            settings->orderIds.begin() + end);
        vector<OrderInfo> orderInfo(settings->orderInfo.begin() + start,
            settings->orderInfo.begin() + end);

        auto range = getRange(orderIds, orderInfo);

        if (fPlace->getFixed() && !range.isInside(fPlace->getD())) {
            effort = numeric_limits<double>::infinity();
            return make_pair(vector<double>(), false);
        }
        if (range.isEmpty()) {
            return make_pair(vector<double>(), false);
        }
        if (!fPlace->getFixed()) {
            double d = range.sample();
            fPlace->setD(d);
        }

        orderIds.erase(orderIds.begin());
        orderInfo.erase(orderInfo.begin());
        setPlacements(orderIds, orderInfo);
    }

    vector<double> positions;
    for (auto* vertex : freeVertices) {
        int id = vertex->getId();
        auto position = settings->vertexPlacements[id]->getPosition();
        for (int j = 0; j < 3; j++) {
            positions.push_back(position.getValue(j));
        }
    }

    return make_pair(positions, true);
}

bool RuleApplier::sampleSolutionSpace() {
    timer->start("Sample Solutions");

    effort = 0;
    auto limits = findLimits();

    while (true) {
        if (effort > maxEffort) {
            timer->stop("Sample Solutions");
            return false;
        }

        auto [positions, success] = sampleFaceCentric();
        bool violated = !success || hasViolations(positions, limits);

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

vector<MorphismPath*> RuleApplier::getFreeablePaths() const {
    vector<MorphismPath*> result;
    copy_if(openPaths.begin(), openPaths.end(), back_inserter(result),
        [](MorphismPath* path) {
            return path->halfEdges[0] != path->halfEdges[1] &&
                path->extendableness() > 0;
        });
    return result;
}

void RuleApplier::freeVertex() {
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

void RuleApplier::freeOneVertex(Vertex* vertex) {
    auto vertexHalfEdges = vertex->getHalfEdges();

    // Process all vertex halfEdges
    for (auto* vHalfEdge : vertexHalfEdges) {
        auto* edge = vHalfEdge->getEdge();
        // Add edge if it's not already in edges.
        if (std::find(edges.begin(), edges.end(), edge) == edges.end()) {
            addEdge(edge, true);
        }
    }

    // Process paths for each halfEdge
    for (auto* vHalfEdge : vertexHalfEdges) {
        auto* edge = vHalfEdge->getEdge();
        
        auto findPath0 = [vHalfEdge](MorphismPath* path) { 
            return path->halfEdges[0] == vHalfEdge; 
        };
        auto findPath1 = [vHalfEdge](MorphismPath* path) { 
            return path->halfEdges[1] == vHalfEdge; 
        };

        auto path0 = find_if(openPaths.begin(), openPaths.end(), findPath0);
        auto path1 = find_if(openPaths.begin(), openPaths.end(), findPath1);

        if (path0 != openPaths.end() && path1 != openPaths.end()) {
            if (*path0 == *path1) {
                // Same path
                (*path0)->halfEdges.clear();
                Util::remove(openPaths, *path0);
            } else {
                // Different paths
                (*path1)->merge(*path0);
                Util::remove(openPaths, *path0);
            }
        } else if (path0 != openPaths.end()) {
            (*path0)->expandBackward();
        } else if (path1 != openPaths.end()) {
            (*path1)->expandForward();
        } else {
            // Create new path
            auto* path = new MorphismPath({}, &edges);
            path->setHalfEdges({vHalfEdge, vHalfEdge});
            path->expandBackward();
            path->expandForward();
            openPaths.push_back(path);
        }
    }
}

bool RuleApplier::placeVertexPositions(const vector<double>& positions) {
    // Place vertices at their new positions
    for (size_t i = 0; i < freeVertices.size(); i++) {
        Vec3 position(
            positions[3 * i], 
            positions[3 * i + 1], 
            positions[3 * i + 2]
        );
        freeVertices[i]->setPosition(position);
        /*if (!model->inBounds(position.x, position.y, position.z)) {
            return false;
        }*/

    }
    if (dims == 2) {
        for (int i = 0; i < (int)edges.size(); i++) {
            bool success = edges[i]->addToBsp();
            if (!success) {
                for (int j = i - 1; j >= 0; j--) {
                    edges[j]->removeFromBsp();
                }
                return false;
            }
        }
        return true;
    }

    vector<Face*> facesToIntersect;
    for (const auto& [_, fPlace] : settings->facePlacements) {
        if (fPlace->getFace()) {
            facesToIntersect.push_back(fPlace->getFace());
        }
    }

    // TODO: Handle holes.
    //// Check face containment
    //bool success = all_of(facesToIntersect.begin(), facesToIntersect.end(),
    //    [this](Face* faceA) {
    //        auto faces = faceA->getGroup()->getFaces();
    //        if (faces.size() > 1) {
    //            if (faceA->isHole()) {
    //                if (!faces[0]->containsFace(faceA)) {
    //                    return false;
    //                }
    //                if (GlobalSettings::get("Cut Holes")) {
    //                    return all_of(faces.begin(), faces.end(),
    //                        [faceA](Face* faceB) {
    //                            return faceA == faceB || faceB == faces[0] || 
    //                                    faceA->outsideFace(faceB);
    //                        });
    //                }
    //                return true;
    //            } else {
    //                return all_of(faces.begin(), faces.end(),
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

    // I think this is necessary to make sure the old face locations get updated,
    // but I'm not sure.
    for (size_t i = 0; i < facesToIntersect.size(); i++) {
        facesToIntersect[i]->removeFromBsp();
    }
    for (int i = 0; i < (int)facesToIntersect.size(); i++) {
        bool success = facesToIntersect[i]->addToBsp();
        if (!success) {
            for (int j = i - 1; j >= 0; j--) {
                facesToIntersect[j]->removeFromBsp();
            }
            return false;
        }
    }

    return true;
}

void RuleApplier::reject() {
    delete graph;
    graph = nullptr;

    edges.clear();
    freeVertices.clear();
    freeEdges.clear();
    propagationOrder.clear();
    basisEdges.clear();

    initialPosition = nullptr;
    effort = numeric_limits<double>::infinity();
}

