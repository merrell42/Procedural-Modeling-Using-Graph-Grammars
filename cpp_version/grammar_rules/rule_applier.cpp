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

RuleApplier::RuleApplier() {
    MemoryCounter::creation("RuleApplier");
}

RuleApplier::~RuleApplier() {
    MemoryCounter::destruction("RuleApplier");
    for (auto* openPath : openPaths) {
        delete openPath;
    }
    openPaths.clear();
}

unique_ptr<RuleApplier> RuleApplier::build(const Production& production, Model* model, int dims) {
    auto result = make_unique<RuleApplier>();
    result->create(production, model, dims);
    
    if (result->effort > 0) {
        result->reject();
        return nullptr;
    }
    return result;
}

void RuleApplier::create(const Production& production, Model* model_, int dims_) {
    model = model_;
    dims = dims_;
    startGraph = production.startGraph;
    endGraph = production.endGraph;
    morphism = production.morphism;
    ground = production.ground;

    effort = 0;

    // TODO: Merge duplicate edges
    /*if (!mergeDuplicateEdges()) {
        effort = numeric_limits<double>::infinity();
        return;
    }*/

    timer->start("Create Graph");
    graph = createGraph();
    timer->stop("Create Graph");

    if (!graph) {
        effort = numeric_limits<double>::infinity();
        return;
    }
}

void RuleApplier::addEdgeToGraph(Edge* edge) {
    auto edgeHalfEdges = edge->getHalfEdges();
    for (auto* halfEdge : edgeHalfEdges) {
        Util::union_(graph->vertices, {halfEdge->getVertex()});
    }
    graph->edges.push_back(edge);
}

// Destroy edges and the half-edges and vertices they're connected to.
static void destroyEdges(const vector<vector<Edge*>>& edgeGroup) {
    set<HalfEdge*> halfEdgeSet;
    set<Vertex*> vertexSet;
    for (const auto& edgeGroup : edgeGroup) {
        for (Edge* edge : edgeGroup) {
            if (edge) {
                auto halfEdges = edge->getHalfEdges();
                if (halfEdges[0]) {
                    halfEdgeSet.insert(halfEdges[0]);
                }
                if (halfEdges[1]) {
                    halfEdgeSet.insert(halfEdges[1]);
                }
                edge->destroy();
            }
        }
    }
    for (const auto& halfEdge : halfEdgeSet) {
        halfEdge->getFace()->removeHalfEdge(halfEdge);
        vertexSet.insert(halfEdge->getVertex());
        halfEdge->destroy();
    }
    for (const auto& vertex : vertexSet) {
        if (vertex) {
            vertex->destroy();
        }
    }
}

EditGraph* RuleApplier::createGraph() {
    // The merged graph merges the end graph where the start graph was originally.
    EditGraph* merged = new EditGraph();
    
    // Save face locations. This may need to be added back in.
    /*if (morphism->outerFaces) {
        morphism->outerFacesD.clear();
        for (auto* outerFace : morphism->outerFaces) {
            morphism->outerFacesD.push_back(
                outerFace->getFaceType()->getNormal().dot(
                    outerFace->getHalfEdges()[0]->getPosition())); 
        }
    }*/

    // Maps from end graph half-edges to merged half-edges.
    merged->halfEdges.resize(endGraph->getHalfEdges().size(), nullptr);
    auto endToMergedHalf = [&](GraphHalfEdge* endHalf) -> HalfEdge* {
        int index = Util::findIndex<GraphHalfEdge*>(endGraph->getHalfEdges(), endHalf);
        return merged->halfEdges[index];
    };
    auto setEndToMergedHalf = [&](GraphHalfEdge* endHalf, HalfEdge* mergedHalf) {
        int index = Util::findIndex<GraphHalfEdge*>(endGraph->getHalfEdges(), endHalf);
        merged->halfEdges[index] = mergedHalf;
    };
    // Maps from end graph vertices to merged vertices.
    auto endToMergedVertex = [&](GraphVertex* endVertex) -> Vertex* {
        int index = Util::findIndex<GraphVertex*>(endGraph->getVertices(), endVertex);
        return merged->vertices[index];
    };

    // Each entry in splitEdges initially starts with one edge.
    // If an edge is split it is saved in splitEdges as two edges.
    vector<vector<Edge*>> splitEdges;
    vector<vector<HalfEdge*>> splitHalfEdges;
    int numEdges = (int)morphism->edgeBtoA.size();
    splitEdges.resize(numEdges);
    splitHalfEdges.resize(numEdges);
    for (size_t i = 0; i < numEdges; ++i) {
        splitEdges[i] = {morphism->edgeBtoA[i]};
    }

    // Create vertices that are not on the boundary.
    auto& endVertices = endGraph->getVertices();
    const auto numVertices = endVertices.size();
    merged->vertices.resize(numVertices, nullptr);
    for (size_t i = 0; i < numVertices; ++i) {
        auto* v = endVertices[i];

        if (v->boundaryIndex() < 0) {
            auto* type = v->getType();
            auto* newVertex = new Vertex(model, Vec3(0, 0, 0), type);
            newVertex->createHalfEdges();
            merged->vertices[i] = newVertex;

            auto vHalfEdges = newVertex->getHalfEdges();
            auto& halfs = v->getHalfEdges();

            for (size_t j = 0; j < halfs.size(); ++j) {
                setEndToMergedHalf(halfs[j], vHalfEdges[j]);
            }
        }
    }

    struct EdgeData {
        vector<HalfEdge*> mergedHalfs;
        vector<GraphHalfEdge*> endHalfs;
        bool modified;
    };

    // Go through each edge in the end graph, splitting it when it's on the boundary.
    vector<EdgeData> edgeData;
    auto& endEdges = endGraph->getEdges();
    for (size_t i = 0; i < endEdges.size(); ++i) {
        auto* endEdge = endEdges[i];
        auto& edgeHalfs = endEdge->getHalfEdges();
        vector<HalfEdge*> mergedHalfs;
        vector<GraphHalfEdge*> endHalfs;
        bool modified = false;

        // Iterate through each half-edge of the edge.
        for (size_t e = 0; e < edgeHalfs.size(); ++e) {
            auto* endHalf = edgeHalfs[e][0];
            auto* endVertex = endHalf->getVertex();
            auto* mergedVertex = endToMergedVertex(endVertex);

            // If the end vertex is on the boundary. Split the edge in half.
            int boundaryIndex = endVertex->boundaryIndex();
            if (boundaryIndex >= 0) {
                // Find the edge location in the start graph.
                int startIndex = Util::findIndex(startGraph->getEdges(),
                    startGraph->getBVertices()[boundaryIndex]->interiorEdge());

                if (splitEdges[startIndex].size() == 1) {
                    // Split the edge if it hasn't already been split.
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

                modified = true;
                int edgeIndex = endHalf->getForward() ? 0 : 1;
                auto* mergedHalf = splitEdges[startIndex][edgeIndex]->getHalfEdges()[e];
                splitEdges[startIndex][edgeIndex] = nullptr;
                mergedHalfs.push_back(mergedHalf);
                setEndToMergedHalf(endHalf, mergedHalf);
            } else {
                auto* mergedHalf = mergedVertex->getHalfEdges()[endHalf->getVertexIndex()];
                mergedHalfs.push_back(mergedHalf);
            }
            endHalfs.push_back(endHalf);
        }
        edgeData.push_back({mergedHalfs, endHalfs, modified});
    }

    // Add half-edges on the boundary to the merged graph.
    for (size_t i = 0; i < endEdges.size(); i++) {
        GraphEdge* endEdge = endEdges[i];
        auto edgeHalfs = endEdge->getHalfEdges();

        for (size_t e = 0; e < edgeHalfs.size(); e++) {
            GraphHalfEdge* halfNext = edgeHalfs[e][0]->getNext();
            // If half next has no edge, it is on the boundary.
            if (!halfNext->getEdge()) {
                // Map from the boundary of the end graph to the boundary of the start graph.
                int boundaryIndex = indexOf(endGraph->getBHalfEdges(), halfNext);
                auto startHalf = startGraph->getBHalfEdges()[boundaryIndex];
                int startIndex = indexOf(startGraph->getEdges(), startHalf->getPrev()->getEdge());
                setEndToMergedHalf(halfNext, splitHalfEdges[startIndex][e]);
            }
        }
    }

    bool failed = false;

    // For each face on the end graph, find consecutive half-edges convert them to the
    // merged graph and merge the faces together.
    for (GraphFace* endFace : endGraph->getFaces()) {
        auto endHalfs = endFace->getOuterHalfEdges();
        size_t N = endHalfs.size();

        for (size_t i = 0; i < N; i++) {
            GraphHalfEdge* endHalfA = const_cast<GraphHalfEdge*>(endHalfs[i]);
            GraphHalfEdge* endHalfB = const_cast<GraphHalfEdge*>(endHalfs[(i + 1) % N]);
            HalfEdge* mergedHalfA = endToMergedHalf(endHalfA);
            HalfEdge* mergedHalfB = endToMergedHalf(endHalfB);

            if (!mergedHalfA || !mergedHalfB) {
                failed = true;
                continue;
            }
            mergedHalfA->mergeFaces(mergedHalfB);
        }
    }

    if (failed) {
        cout << "Do not know how this can happen, but endToMergedHalf is missing an halfEdge." << endl;
        return merged;
    }

    // Set merged edges using edgeData.
    merged->edges.resize(edgeData.size());
    for (size_t i = 0; i < edgeData.size(); i++) {
        EdgeData& datum = edgeData[i];
        Edge* edge0 = datum.mergedHalfs[0]->getEdge();

        // Edge normally have 2 half-edges, but that's not always true.
        // A new edge is created for each half-edge, but they're really the same.
        // Combine these edges into one.
        for (size_t j = 1; j < datum.mergedHalfs.size(); j++) {
            HalfEdge* halfEdgeJ = datum.mergedHalfs[j];
            Edge* edgeJ = halfEdgeJ->getEdge();
            edge0->addHalfEdge(halfEdgeJ, datum.endHalfs[j]->getEdgeIndex());
            edgeJ->destroy();
        }
        merged->edges[i] = edge0;

        // Include the vertices at both ends of the eget.
        if (datum.modified) {
            vector<Vertex*> newVertices = {
                datum.mergedHalfs[0]->getVertex(),
                datum.mergedHalfs[1]->getVertex()
            };
            Util::union_(merged->vertices, newVertices);
        }
    }

    // Create open paths by starting with the boundary vertices and
    // tracing paths along their faces.
    auto& bVertices = endGraph->getBVertices();
    for (size_t i = 0; i < bVertices.size(); i++) {
        // For each boundary vertex, find a half-edge pointing away from the boundary.
        GraphHalfEdge* interiorHalf = bVertices[i]->interiorHalfEdge();
        if (interiorHalf) {
            // Starting from the boundary, trace a path along the face.
            auto endHalfs = GraphFace::getConnectedHalfEdges(interiorHalf);
            GraphHalfEdge* lastEndHalf = const_cast<GraphHalfEdge*>(endHalfs.back());
            endHalfs.pop_back();

            // Convert to merged half-edges.
            vector<HalfEdge*> mergedHalfs;
            for (GraphHalfEdge* half : endHalfs) {
                mergedHalfs.push_back(endToMergedHalf(half));
            }

            // Create the path.
            MorphismPath* path = MorphismPath::createPath(mergedHalfs);
            HalfEdge* lastMergedHalf = endToMergedHalf(lastEndHalf);
            path->setHalfEdges({ mergedHalfs[0], lastMergedHalf });
            openPaths.push_back(path);
        }
    }

    // Destroy these edges and the vertices and half-edges they connect to. 
    destroyEdges(splitEdges);

    // Add the merged faces by converting the end faces.
    merged->faces.clear();
    for (GraphFace* endFace : endGraph->getFaces()) {
        GraphHalfEdge* endHalf = endFace->getOuterComponent();
        if (endHalf) {
            auto mergedFace = endToMergedHalf(endHalf)->getFace();
            merged->faces.push_back(mergedFace);
        }
    }

    // The boundary face is always a hole. Set hole to true.
    size_t faceIndex = 0;
    for (GraphFace* outerFace : endGraph->getBFaces()) {
        faceIndex = indexOf(endGraph->getFaces(), outerFace);
        merged->faces[faceIndex]->setHole(true);
    }
    // Split the face off from a group if it is not a hole, but is not in
    // outer position. It was split from another face, but is no longer coplanar.
    for (Face* face : merged->faces) {
        if (!face->isHole() &&
            indexOf(face->getGroup()->getFaces(), face) > 0) {
            face->splitGroup();
        }
    }

    // Update outer faces morphism
    //morphism.outerFacesA.clear();
    //for (Face* endFace : endGraph->getOuterFaces()) {
    //    morphism.outerFacesA.push_back(
    //        endToMergedHalf(endFace->getOuterComponent())->getFace());
    //}

    return merged;
}

bool RuleApplier::solve() {
    timer->start("setupForSampling");
    setupForSampling();
    timer->stop("setupForSampling");
    return sampleRepeatedly();
}

// Add constraints to all the vertex placements until they all have 3 constraints.
// Find all vertices with less than 3 constraints and pick a vertex that is the most
// constrained first. Continue until all have 3 constraints.
void RuleApplier::constrainVertexIds(vector<int>& vertexIds, RuleApplierSettings* settings) {
    vector idsToConstrain(vertexIds);
    while (true) {
        vector<int> newIdsToConstrain;
        VertexPlacement* mostConstrained = nullptr;
        int mostConstraints = -1;

        for (int id : idsToConstrain) {
            auto* vPlace = settings->getVertex(id);
            int numConstraints = vPlace->getNumConstraints();

            // Find the vertex placement with the most constraints that are less than 3.
            if (numConstraints < 3) {
                if (mostConstraints < numConstraints) {
                    mostConstraints = numConstraints;
                    mostConstrained = vPlace;
                }
                newIdsToConstrain.push_back(id);
            }
        }

        if (mostConstrained) {
            mostConstrained->addConstraint();
        } else {
            return;
        }

        idsToConstrain = move(newIdsToConstrain);
    }
}

// Connect a new face as an inner component or hole to this one.
void RuleApplier::addFixedFace(Face* fixedFaceA, Face* fixedFaceB, double d) {
    auto* fPlace = settings->getFace(fixedFaceB->getId());

    // Check if face is already fixed.
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

void RuleApplier::setupForSampling() {
    auto extents = getExtents();
    const Vec3 lower(1, 1, 0);
    const Vec3 upper(extents[0] - 1, extents[1] - 1, extents[2]);
    auto settingsNew = make_unique<RuleApplierSettings>(lower, upper);
    this->settings = move(settingsNew);
    freeVertices.clear();

    vector<int> basisIds;
    vector<int> vertexIds;

    // Create an edge placement for each edge.
    for (auto* edge : graph->edges) {
        int id = edge->getId();
        settings->edgePlacements[id] = make_unique<EdgePlacement>(edge, id, settings.get());
    }

    // Create an vertex placement for each vertex.
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

    // Initialize edge placements.
    for (auto* edge : graph->edges) {
        int id = edge->getId();
        settings->edgePlacements[id]->initialize();
    }

    // Guarantee that all vertex placements have three face placements.
    for (int id : vertexIds) {
        settings->getVertex(id)->guaranteeThreeFaces();
    }

    // Open paths have a fixed vertex at each end.
    // Add their IDs to fixedVertexIDs.
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

    // Find all fixed faces.
    fixedFaces.clear();
    auto& faceBtoA = morphism->faceBtoA;
    
    for (size_t i = 0; i < faceBtoA.size(); ++i) {
        auto* fixedFaceA = faceBtoA[i];
        auto* endFace = endGraph->getBFaces()[i];
        
        if (endFace) {
            int faceIndex = indexOf(endGraph->getFaces(), endFace);
            auto* fixedFaceB = graph->faces[faceIndex];
            double d;

            // TODO: Add this back in.
            // Use the value from outerFacesA if fixedFaceA is destroyed.
            /*if (fixedFaceA->getNode()->isDestroyed()) {
                fixedFaceA = morphism->outerFacesA[i];
                d = morphism->outerFacesD[i];
            } else {*/
                auto normal = fixedFaceB->getFaceType()->getNormal();
                auto halfEdges = fixedFaceA->getHalfEdges();
                auto vPosition = halfEdges[0]->getVertex()->getPosition();
                d = normal.dot(vPosition);
            // }

            addFixedFace(fixedFaceA, fixedFaceB, d);
        }
    }

    // Add fixed faces for face groups with holes in them.
    for (auto& [_, fPlace] : settings->facePlacements) {
        auto face = fPlace->getFace();
        if (face) {
            auto* group = face->getGroup();
            if (group->getFaces().size() > 1) {
                auto normal = face->getFaceType()->getNormal();
                auto halfEdges = face->getHalfEdges();
                auto vPosition = halfEdges[0]->getVertex()->getPosition();
                double d = normal.dot(vPosition);
                addFixedFace(face, face, d);
            }
        }
    }

    // Constrain all the vertices until they all have three constraints.
    constrainVertexIds(fixedVertexIds, settings.get());
    constrainVertexIds(vertexIds, settings.get());
}

// Determine if any positions are outside the extent of the model we are generating.
bool RuleApplier::outOfBounds(const vector<double>& positions) {
    auto extents = getExtents();
    for (size_t i = 0; i < positions.size(); i++) {
        double value = positions[i];
        int dim = i % 3;
        if (value < 0 || value > extents[dim]) {
            return true;
        }
    }
    return false;
}

// Find the range of d-values for one face placement. Since face placement may determine
// the position of some vertices and edges, those positions constrain the d-values more.
// We take the intersect of the acceptable ranges for all affected faces, edges, and vertices.
Range RuleApplier::getRange(
    const vector<int>& orderIds,
    const vector<OrderInfo>& orderInfo,
    int startIndex,
    int endIndex
) {
    Range range(-numeric_limits<double>::infinity(),
        numeric_limits<double>::infinity());

    for (size_t i = startIndex; i < endIndex; i++) {
        int id = orderIds[i];
        const auto& info = orderInfo[i];

        Range rangeI;
        switch (info.type) {
            case OrderInfo::Type::Vertex:
                rangeI = settings->getVertex(id)->getRange();
                break;
            case OrderInfo::Type::Edge:
                rangeI = settings->getEdge(id)->getRange();
                break;
            case OrderInfo::Type::Face:
                rangeI = settings->getFace(id)->getRange(info.vertexId);
                break;
        }
        range.intersect(rangeI);

        // Early exit if range is empty.
        if (range.isEmpty()) {
            return range;
        }
    }

    return range;
}

// Update the vertex and face placements. The edge placements can be ignored.
void RuleApplier::setPlacements(
    const vector<int>& orderIds,
    const vector<OrderInfo>& orderInfo,
    int startIndex,
    int endIndex
) {
    for (size_t i = startIndex; i < endIndex; i++) {
        int id = orderIds[i];
        const auto& info = orderInfo[i];
        switch (info.type) {
            case OrderInfo::Type::Vertex:
                settings->getVertex(id)->setPosition();
                break;
            case OrderInfo::Type::Face:
                settings->getFace(id)->setFromVertex(info.vertexId);
                break;
        }
    }
}

pair<vector<double>, bool> RuleApplier::createGroundPlane() {
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
    } else {
        cout << "Ground rules must have 4 vertices." << endl;
        return {};
    }
}

pair<vector<double>, bool> RuleApplier::sampleSolutionSpace() {
    if (ground) {
        return createGroundPlane();
    }

    // Set the fixed faces and fixed vertices.
    for (const auto& fixed : fixedFaces) {
        fixed.fPlace->setD(fixed.d);
        fixed.fPlace->setFixed(true);
    }
    for (int id : fixedVertexIds) {
        auto success = settings->getVertex(id)->fixPosition();
        if (!success) {
            return {};
        }
    }

    vector<int> basisIndices;
    for (const auto& basisId : settings->basisIds) {
        basisIndices.push_back(settings->findBasisIndex(basisId));
    }
    basisIndices.push_back((int)settings->orderIds.size());

    // timer->start("Set Placements");
    for (size_t i = 0; i < settings->basisIds.size(); i++) {
        int id = settings->basisIds[i];
        auto* fPlace = settings->facePlacements[id].get();

        // Find the acceptable ranges for all the placements between two consecutive basis indices.
        int start = basisIndices[i];
        int end = basisIndices[i + 1];
        auto range = getRange(settings->orderIds, settings->orderInfo, start, end);

        if (range.isEmpty()) {
            // timer->stop("Set Placements");
            return make_pair(vector<double>(), false);
        }

        if (fPlace->getFixed()) {
            // If the face placement is already fixed make sure it is within the acceptable range.
            if (!range.isInside(fPlace->getD())) {
                effort = numeric_limits<double>::infinity();
                // timer->stop("Set Placements`");
                return make_pair(vector<double>(), false);
            }
        } else {
            // Otherwise pick a value within the range.
            double d = range.sample();
            fPlace->setD(d);
        }
        setPlacements(settings->orderIds, settings->orderInfo, start + 1, end);
    }
    // timer->stop("Set Placements");

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

// Sample solution until once is successful or we pass the maximum effort.
bool RuleApplier::sampleRepeatedly() {
    timer->start("Sample Repeatedly");

    effort = 0;
    while (true) {
        if (effort > maxEffort) {
            timer->stop("Sample Repeatedly");
            return false;
        }
        auto [positions, success] = sampleSolutionSpace();
        bool violated = !success || outOfBounds(positions);

        if (!violated) {
            timer->stop("Sample Repeatedly");
            timer->start("Place Vertices");

            if (placeVertexPositions(positions)) {
                timer->stop("Place Vertices");
                return true;
            }

            timer->stop("Place Vertices");
            timer->start("Sample Repeatedly");
        }
        effort++;
    }
}

// Return all paths that are extendable and not in a loop.
vector<MorphismPath*> RuleApplier::getFreeablePaths() const {
    vector<MorphismPath*> result;
    copy_if(openPaths.begin(), openPaths.end(), back_inserter(result),
        [](MorphismPath* path) {
            return path->halfEdges[0] != path->halfEdges[1] &&
                path->isExtendable();
        });
    return result;
}

Vertex* RuleApplier::pickVertexToFree() {
    auto freeablePaths = getFreeablePaths();
    if (freeablePaths.empty()) {
        effort++;
        return nullptr;
    }

    // Pick a random vertex from a random path.
    auto* path = Util::pick(freeablePaths);
    return path->randomNextVertex();
}

void RuleApplier::freeVertex(Vertex* vertex) {
    if (!vertex) {
        return;
    }
    freeVertexBase(vertex);

    bool done = false;
    while (!done) {
        done = true;
        auto freeablePaths = getFreeablePaths();

        // Free vertices that are rigidly attach to the vertex that was freed.
        for (auto* path : freeablePaths) {
            if (auto* rigidVertex = path->rigidNextVertex()) {
                freeVertexBase(rigidVertex);
                done = false;
                break;
            }
        }
    }
}

void RuleApplier::freeVertexBase(Vertex* vertex) {
    auto vertexHalfEdges = vertex->getHalfEdges();

    // Add any adjacent edges that have not already been added to graph->edges.
    for (auto* vHalfEdge : vertexHalfEdges) {
        auto* edge = vHalfEdge->getEdge();
        if (!contains(graph->edges, edge)) {
            addEdgeToGraph(edge);
        }
    }

    // Update the open paths for each half-edge of the vertex.
    for (auto* vHalfEdge : vertexHalfEdges) {
        auto* edge = vHalfEdge->getEdge();

        // Find the paths that start and end with vHalfEdge.
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
                // The freed section now forms a complete loop around the face. The entire
                // face is free. Remove it from openPaths.
                delete* path0;
                Util::remove(openPaths, *path0);
            } else {
                // We have freed two sections of the same face which are now touching at a vertex
                // that was just freed. Merge the two sections into one freed section.
                (*path1)->merge(*path0);
                delete* path0;
                Util::remove(openPaths, *path0);
            }
        } else if (path0 != openPaths.end()) {
            // If vHalfEdge is at the start of a freed section expand it backwards from the start.
            (*path0)->expandBackward();
        } else if (path1 != openPaths.end()) {
            // If vHalfEdge is at the end of a freed section expand it forward from the end.
            (*path1)->expandForward();
        } else {
            // Create new path at vHalfEdge. Create a freed section expanded once in each
            // direction since the vertex is free now.
            auto* path = new MorphismPath({});
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
    // In 2D, check for edge intersections.
    if (dims == 2) {
        for (int i = 0; i < (int)graph->edges.size(); i++) {
            bool success = graph->edges[i]->addToBsp();
            if (!success) {
                for (int j = i - 1; j >= 0; j--) {
                    graph->edges[j]->removeFromBsp();
                }
                return false;
            }
        }
        return true;
    }

    // Find all faces we need to test for intersections.
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
    // Check if each face intersects any faces in the BSP tree.
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

    freeVertices.clear();

    effort = numeric_limits<double>::infinity();
}

