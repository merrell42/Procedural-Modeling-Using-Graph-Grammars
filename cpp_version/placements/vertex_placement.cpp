#include "pch.h"
#include "vertex_placement.h"
#include "../util/util.h"

void VertexPlacement::initialize() {
    vector<HalfEdge*> halfEdges = vertex->getHalfEdges();

    for (auto* halfEdge : halfEdges) {
        if (!halfEdge) {
            // Happens in the ground rule.
            continue;
        }
        Face* face = halfEdge->getFace();
        int id = face->getId();

        // Create the face placement if it doesn't exist.
        if (!settings->getFace(id)) {
            auto normal = face->getFaceType()->getNormal();
            settings->facePlacements[id] = make_unique<FacePlacement>(normal, id, settings, face);
        }
        addFreeFace(id);
    }
}

// Constrain the first free face.
void VertexPlacement::addConstraint() {
    if (freeFaceIds.empty()) {
        throw runtime_error("No free faces available for constraint");
    }
    int freeFaceId = freeFaceIds[0];
    settings->getFace(freeFaceId)->constrain(true, -1);
}

// Remove the face ID from the free faces and add to the constrained one.
void VertexPlacement::constrainFace(int id) {
    Util::remove(freeFaceIds, id);
    constrainedFaceIds.push_back(id);

    // Check if the face IDs are colinear.
    if (constrainedFaceIds.size() == 3) {
        auto* M = getM();
        if (!M) {
            constrainedFaceIds.pop_back();
            colinearFaceIds.push_back(id);
        }
    }
}

// If the position of the vertex is full constrained, propagate its position to the
// neighboring geometry.
void VertexPlacement::propagate() {
    if (constrainedFaceIds.size() >= 3) {
        settings->addToOrder(this->id, OrderInfo::Type::Vertex, -1);
        
        // Handle free faces and colinear faces.
        auto freeIds = freeFaceIds;
        freeIds.insert(freeIds.end(), colinearFaceIds.begin(), colinearFaceIds.end());
        
        for (int faceId : freeIds) {
            auto* fPlace = settings->getFace(faceId);
            if (!fPlace->isConstrained()) {
                fPlace->constrain(false, this->id);
            }
        }

        // Handle connected edges.
        for (HalfEdge* halfEdge : vertex->getHalfEdges()) {
            int edgeId = halfEdge->getEdge()->getId();
            auto* ePlace = settings->getEdge(edgeId);
            if (ePlace) {
                ePlace->addConstraint(this->id);
            }

            auto* prev = halfEdge->prev();
            if (prev->getEdgeType()->faceData.size() == 1) {
                // If prev edge only has one halfEdge, propagate to previous vertex.
                int prevEdgeId = prev->getEdge()->getId();
                auto* prevEPlace = settings->getEdge(prevEdgeId);
                if (prevEPlace) {
                    prevEPlace->addConstraint(this->id);
                }
            }
        }
    }
}

Matrix* VertexPlacement::getA(const vector<int>& faceIds) {
    vector<vector<double>> A(3, vector<double>(3));

    for (size_t i = 0; i < 3; i++) {
        int id = faceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        Vec3 n = fPlace->getNormal();
        A[i] = { n.getX(), n.getY(), n.getZ()};
    }

    return new Matrix(A);
}

Matrix* VertexPlacement::getM() {
    if (!M) {
        Matrix* A = getA(constrainedFaceIds);
        // The three faces are colinear.
        // Return null if A is not invertible
        if (abs(Matrix::det(*A)) < 1e-8) {
            delete A;
            return nullptr;
        }
        M = Matrix::inverse(A);
        delete A;
    }
    return M;
}

void VertexPlacement::setPosition() {
    vector<vector<double>> D(3, vector<double>(1));

    // The position is at the intersection of three planes.
    // Using the d-values for each plane, find the intersection point.
    for (size_t i = 0; i < 3; i++) {
        int id = constrainedFaceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        D[i][0] = fPlace->getD();
    }

    // b = M * D.
    Matrix* DMatrix = new Matrix(D);
    auto bMatrix = Matrix::multiply(getM(), DMatrix);
    auto b = bMatrix->valueOf();
    delete DMatrix;

    slope = Vec3(0, 0, 0);
    value = Vec3(b[0][0], b[1][0], b[2][0]);
    delete bMatrix;
}

Range VertexPlacement::getRange() {
    double m3[3], b3[3];
    for (size_t i = 0; i < 3; i++) {
        int id = constrainedFaceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        auto mbi = fPlace->getChangeMB();
        m3[i] = mbi.m;
        b3[i] = mbi.b;
    }

    auto M = getM()->valueOf();
    double m[3] = {0}, b[3] = {0};
    // Multiply M times m3 and b3.
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m[i] += M[i][j] * m3[j];
            b[i] += M[i][j] * b3[j];
        }
    }
    Range range(-numeric_limits<double>::infinity(), numeric_limits<double>::infinity());
    for (int i = 0; i < 3; i++) {
        Range rangeI = Range::transformCreate(m[i], b[i], Range(settings->lower[i], settings->upper[i]));
        range.intersect(rangeI);
    }

    slope = Vec3(m[0], m[1], m[2]);
    value = Vec3(b[0], b[1], b[2]);

    return range;
}

ChangeVecMB VertexPlacement::getChangeMB() const {
    auto m = slope;
    auto b = value;
    return { m, b };
}

const Vec3& VertexPlacement::getPosition() const {
    return value;
}

void VertexPlacement::addFixedNeighbor(const FixedFace& fixedFace) {
    auto faceIds = getAllFaceIds();
    for (int id : faceIds) {
        if (id != fixedFace.fPlace->id) {
            settings->getFace(id)->addFixedNeighbor(fixedFace.faceA);
        }
    }
}

void VertexPlacement::addFreeFace(int id) {
    // Update id if a new ID is found.
    auto it = settings->uniqueFaceMap.find(id);
    if (it != settings->uniqueFaceMap.end()) {
        id = it->second;
    }

    // Check if the ID is already in freeFaceIds.
    if (find(freeFaceIds.begin(), freeFaceIds.end(), id) != freeFaceIds.end()) {
        return;
    }

    FacePlacement* newFreeFace = settings->getFace(id);
    int coplanarId = 0;
    bool isCoplanar = false;
    for (int existingId : freeFaceIds) {
        FacePlacement* existingFace = settings->getFace(existingId);
        if (newFreeFace->coplanar(existingFace)) {
            coplanarId = existingId;
            isCoplanar = true;
            break;
        }
    }

    if (isCoplanar) {
        settings->mergeFace(coplanarId, id);
        id = coplanarId;
    } else {
        freeFaceIds.push_back(id);
    }

    settings->getFace(id)->addVertexId(this->id); // Add the vertex ID to the face

    // Check for colinearity
    //if (freeFaceIds.size() == 3 && getA(freeFaceIds).empty()) {
    //    freeFaceIds.pop_back(); // Remove the last face ID
    //}
}

vector<int> VertexPlacement::getAllFaceIds() const {
    vector<int> allFaceIds;
    allFaceIds.reserve(constrainedFaceIds.size() + freeFaceIds.size() + colinearFaceIds.size());

    // Concatenate the vectors.
    allFaceIds.insert(allFaceIds.end(), constrainedFaceIds.begin(), constrainedFaceIds.end());
    allFaceIds.insert(allFaceIds.end(), freeFaceIds.begin(), freeFaceIds.end());
    allFaceIds.insert(allFaceIds.end(), colinearFaceIds.begin(), colinearFaceIds.end());

    return allFaceIds; // Return the combined vector.
}

int VertexPlacement::getNumConstraints() const {
    return (int)constrainedFaceIds.size();
}

// Add a third face for leaf vertices.
// Leaf vertices are only connected to one edge.
// The edge gives one constraint.
// They are on a plane which gives another constraint.
// Add a third constraint by adding a fake face orthogonal to both.
void VertexPlacement::guaranteeThreeFaces() {
    if (freeFaceIds.size() < 2) {
        cout << "Vertex should have at least two faces.\n";
    } else if (freeFaceIds.size() == 2) {
        Vec3 n0 = settings->getFace(freeFaceIds[0])->getNormal();
        Vec3 n1 = settings->getFace(freeFaceIds[1])->getNormal();
        Vec3 n2 = n0.cross(n1);
        n2.normalize();
        int newId = settings->createFace(n2);
        addFreeFace(newId);
    }
}

bool VertexPlacement::fixPosition() {
    this->slope = Vec3::ORIGIN;
    this->value = this->vertex->getPosition();

    for (const auto& id : this->getAllFaceIds()) {
        auto success = settings->getFace(id)->setFromVertex(this->id);
        if (!success) {
            return false;
        }
        settings->getFace(id)->setFixed(true);
    }

    return true;
}
