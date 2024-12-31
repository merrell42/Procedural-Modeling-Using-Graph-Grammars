#include "classifier.h"
#include "util.h"
#include "network_factory.h"
#include <cmath>

namespace ms {

float Classifier::ANGLE_TOLERANCE = 10.0f / 180.0f * M_PI;
float Classifier::FLIP_ANGLE = 53.0f / 180.0f * M_PI;

Classifier::Classifier()
    : edges()
    , hierarchy(nullptr)
    , boundaryGroups()
    , fullyConnected(false)
    , is3D(false)
    , xml(nullptr) {
    resetHierarchy();
}

Classifier::~Classifier() = default;

void Classifier::importSolution(const Solution& solution) {
    if (solution.matches) {
        if (solution.xml) {
            std::string decorationText = solution.xml;
            xml = Util::xmlTextToJson(decorationText);
            xml->decorationText = decorationText;
        }
        hierarchy = NetworkHierarchy::partialImport(solution, xml);
    } else if (solution.useNetworks) {
        hierarchy = NetworkHierarchy::import(solution);
    } else {
        hierarchy = FamilyTree::import(solution);
    }
}

void Classifier::resetHierarchy() {
    if (globalSettings.getBool("Use Network")) {
        hierarchy = std::make_unique<NetworkHierarchy>();
    } else {
        hierarchy = std::make_unique<FamilyTree>();
    }
}

EdgeDatum* Classifier::findMatchingEdge(const EdgeDatum& edgeDatumA, 
                                      const std::vector<EdgeDatum>& edgeDataB,
                                      bool isRigidFixed) {
    auto matchingAreas = [](const Area* a, const Area* b) {
        return a->leftArea == b->leftArea && a->rightArea == b->rightArea;
    };

    if (isRigidFixed) {
        for (auto& edgeDatumB : edgeDataB) {
            if (!matchingAreas(&edgeDatumA, &edgeDatumB)) {
                continue;
            }
            if (edgeDatumA.shapeEdge == edgeDatumB.shapeEdge) {
                return &edgeDatumB;
            }
            auto shapeA = edgeDatumA.shapeEdge->shape;
            auto shapeB = edgeDatumB.shapeEdge->shape;
            if (shapeA->getGroup() != shapeB->getGroup()) {
                continue;
            }
            auto startA = edgeDatumA.shapeEdge->start->vertex;
            auto startB = edgeDatumB.shapeEdge->start->vertex;
            auto endA = edgeDatumA.shapeEdge->end->vertex;
            auto endB = edgeDatumB.shapeEdge->end->vertex;
            if (shapeA->getVertexIndex(startA) == shapeB->getVertexIndex(startB) &&
                shapeA->getVertexIndex(endA) == shapeB->getVertexIndex(endB)) {
                return &edgeDatumB;
            }
        }
    } else {
        for (auto& edgeDatumB : edgeDataB) {
            if (edgeDatumA.brush != edgeDatumB.brush || !matchingAreas(&edgeDatumA, &edgeDatumB)) {
                continue;
            }
            bool classified = edgeDatumA.brush ? edgeDatumA.brush->get("Classified by angle") : true;
            if (classified) {
                if (std::abs(Util::angleDifference(edgeDatumA.angle, edgeDatumB.angle)) < ANGLE_TOLERANCE) {
                    return &edgeDatumB;
                }
            } else {
                return &edgeDatumB;
            }
        }
    }
    return nullptr;
}

void Classifier::setShapes(const std::vector<Shape*>& shapes) {
    is3D = shapes[0]->is3D;
    Primitives primitives;

    if (is3D) {
        auto fileName = shapes[0]->getName();
        auto decorationText = ms::obj[fileName].xml;
        auto decoration = Util::xmlTextToJson(decorationText);
        auto objFile = OBJFile(ms::obj[fileName].obj, fileName);
        objFile.parse();
        primitives = GraphFinder3D::find(objFile.result.models[0], decoration.decoration);
        xml = primitives.xml;
        xml->decorationText = decorationText;
    } else {
        primitives = extractPrimitives2D(shapes);
        primitives.splicedEdgeTypes = {};
        primitives.getSpliceEdgeType = [&](FaceType* faceType, const Vec3& dir) {
            return Shape3D::getSpliceEdgeType(primitives.splicedEdgeTypes, 
                                            primitives.edgeTypes, 
                                            faceType, dir);
        };
    }

    resetHierarchy();
    hierarchy->generate(primitives);
    boundaryGroups = hierarchy->boundaryGroups;
    
    fullyConnected = std::any_of(primitives.edgeTypes.begin(), 
                                primitives.edgeTypes.end(),
                                [](EdgeType* edgeType) { 
                                    return edgeType->isConnected(); 
                                });
}

// ... Additional method implementations ...

} // namespace ms 