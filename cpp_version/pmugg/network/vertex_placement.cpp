#include "pch.h"
#include "vertex_placement.h"

namespace ms {

void VertexPlacement::initialize() {
    std::vector<Endpoint*> endpoints = vertex->getEndpoints();

    for (auto* endpoint : endpoints) {
        if (!endpoint) {
            // Happens in the ground transition.
            continue;
        }
        Face* face = endpoint->getFace();
        int id = face->getId();

        if (!settings->getFace(id)) {
            auto normal = face->getFaceType()->getNormal();
            settings->facePlacements[id] = std::make_unique<FacePlacement>(normal, id, settings, face);
        }
        addFreeFace(id);
    }
}

void VertexPlacement::addConstraint() {
    if (freeFaceIds.empty()) {
        throw std::runtime_error("No free faces available for constraint");
    }
    int freeFaceId = freeFaceIds[0];
    settings->getFace(freeFaceId)->constrain(true, -1);
}

void VertexPlacement::constrainFace(int id) {
    auto it = std::find(freeFaceIds.begin(), freeFaceIds.end(), id);
    if (it != freeFaceIds.end()) {
        freeFaceIds.erase(it);
    }
    unfreeFaceIds.push_back(id);

    // Check if the face IDs are colinear
    if (unfreeFaceIds.size() == 3) {
        auto* M = getM();
        if (!M) {
            unfreeFaceIds.pop_back();
            colinearFaceIds.push_back(id);
        }
    }
}

void VertexPlacement::propagate() {
    if (unfreeFaceIds.size() >= 3) {
        settings->addToOrder(this->id, "vertex", -1);
        
        // Handle free faces and colinear faces
        auto freeIds = freeFaceIds;
        freeIds.insert(freeIds.end(), colinearFaceIds.begin(), colinearFaceIds.end());
        
        for (int faceId : freeIds) {
            auto* fPlace = settings->getFace(faceId);
            if (fPlace->isFree()) {
                fPlace->constrain(false, this->id);
            }
        }

        // Handle connected edges
        for (Endpoint* endpoint : vertex->getEndpoints()) {
            int edgeId = endpoint->getLine()->getId();
            auto* ePlace = settings->getEdge(edgeId);
            if (ePlace) {
                ePlace->addConstraint(this->id);
            }

            auto* prev = endpoint->prev();
            if (prev->getEdgeType()->faceData.size() == 1) {
                // If prev edge only has one endpoint, propagate to previous vertex
                int prevEdgeId = prev->getLine()->getId();
                auto* prevEPlace = settings->getEdge(prevEdgeId);
                if (prevEPlace) {
                    prevEPlace->addConstraint(this->id);
                }
            }
        }
    }
}

Matrix* VertexPlacement::getA(const std::vector<int>& faceIds) {
    std::vector<std::vector<double>> A(3, std::vector<double>(3)); // Assuming A is a 3x3 matrix

    for (size_t i = 0; i < 3; i++) {
        int id = faceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        Vec3 n = fPlace->getNormal();
        A[i] = { n.getX(), n.getY(), n.getZ()}; // Fill the matrix row with the normal components
    }

    return new Matrix(A);
}

Matrix* VertexPlacement::getM() {
    if (!M) {
        Matrix* A = getA(unfreeFaceIds);
        // The three faces are collinear.
        if (std::abs(Matrix::det(*A)) < 1e-8) {
            delete A; // Clean up memory
            return nullptr; // Return null if A is not invertible
        }
        M = Matrix::inverse(A);
        delete A; // Clean up memory for A
    }
    return M;
}

void VertexPlacement::setPosition() {
    std::vector<std::vector<double>> D(3, std::vector<double>(1)); // 3x1 matrix

    for (size_t i = 0; i < 3; i++) {
        int id = unfreeFaceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        D[i][0] = fPlace->getD(); // Fill D with D values
    }

    Matrix* DMatrix = new Matrix(D); // Create a matrix from D
    auto bMatrix = Matrix::multiply(getM(), DMatrix); // Multiply M and D
    auto b = bMatrix->valueOf(); // Multiply M and D
    delete DMatrix; // Clean up memory for DMatrix

    slope = Vec3(0, 0, 0); // Initialize slope
    value = Vec3(b[0][0], b[1][0], b[2][0]); // Assuming b is a 3x1 matrix
    delete bMatrix; // Clean up memory for bMatrix
}

Range VertexPlacement::getRange() {
    std::vector<std::vector<double>> m3(3, std::vector<double>(1));
    std::vector<std::vector<double>> b3(3, std::vector<double>(1));

    for (size_t i = 0; i < 3; i++) {
        int id = unfreeFaceIds[i];
        FacePlacement* fPlace = settings->getFace(id);
        auto mbi = fPlace->getChangeMB();
        m3[i][0] = mbi.m; // Assuming mbi.m is a Vec3 or similar
        b3[i][0] = mbi.b; // Assuming mbi.b is a double
    }

    Matrix* m3Matrix = new Matrix(m3);
    Matrix* b3Matrix = new Matrix(b3);
    Matrix* mMatrix = Matrix::multiply(getM(), m3Matrix);
    Matrix* bMatrix = Matrix::multiply(getM(), b3Matrix);
    auto m = mMatrix->valueOf();
    auto b = bMatrix->valueOf();

    delete m3Matrix; // Clean up memory for m3Matrix
    delete b3Matrix; // Clean up memory for b3Matrix

    Range range(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    for (size_t i = 0; i < 3; i++) {
        Range rangeI = Range::transformCreate(m[i][0], b[i][0], Range(settings->lower[i], settings->upper[i]));
        range = range.intersect(rangeI);
    }

    slope = Vec3(m[0][0], m[1][0], m[2][0]);
    value = Vec3(b[0][0], b[1][0], b[2][0]);

    delete mMatrix; // Clean up memory for m
    delete bMatrix; // Clean up memory for b

    return range;
}

ChangeVecMB VertexPlacement::getChangeMB() const {
    auto m = slope;
    auto b = value;
    return { m, b };
}

const Vec3& VertexPlacement::getPosition() const {
    return value; // Return the position value
}

void VertexPlacement::addFixedNeighbor(const FixedFace& fixedFace) {
    auto faceIds = getAllFaceIds(); // Get all face IDs
    for (int id : faceIds) {
        if (id != fixedFace.fPlace->id) { // Assuming Face has a method getId()
            settings->getFace(id)->addFixedNeighbor(fixedFace.faceA); // Assuming faceA is accessible
        }
    }
}

void VertexPlacement::addFreeFace(int id) {
    auto it = settings->uniqueFaceMap.find(id);
    if (it != settings->uniqueFaceMap.end()) {
        id = it->second; // Update id if a new ID is found
    }

    // Check if the ID is already in freeFaceIds
    if (std::find(freeFaceIds.begin(), freeFaceIds.end(), id) != freeFaceIds.end()) {
        return; // ID already exists, exit the function
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
        id = coplanarId; // Update id to the merged face ID
    } else {
        freeFaceIds.push_back(id); // Add the new face ID
    }

    settings->getFace(id)->addVertexId(this->id); // Add the vertex ID to the face

    // Check for colinearity
    //if (freeFaceIds.size() == 3 && getA(freeFaceIds).empty()) { // Assuming getA returns a vector
    //    freeFaceIds.pop_back(); // Remove the last face ID
    //}
}

std::vector<int> VertexPlacement::getAllFaceIds() const {
    std::vector<int> allFaceIds; // Vector to hold all face IDs

    // Reserve space to avoid multiple allocations
    allFaceIds.reserve(unfreeFaceIds.size() + freeFaceIds.size() + colinearFaceIds.size());

    // Concatenate the vectors
    allFaceIds.insert(allFaceIds.end(), unfreeFaceIds.begin(), unfreeFaceIds.end());
    allFaceIds.insert(allFaceIds.end(), freeFaceIds.begin(), freeFaceIds.end());
    allFaceIds.insert(allFaceIds.end(), colinearFaceIds.begin(), colinearFaceIds.end());

    return allFaceIds; // Return the combined vector
}

// Method to get the number of constraints
int VertexPlacement::getNumConstraints() const {
    return unfreeFaceIds.size();
}


void VertexPlacement::checkThreeFaces() {
    if (freeFaceIds.size() < 2) {
        std::cerr << "Vertex should have at least two faces.\n";
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

    bool success = true;
    for (const auto& id : this->getAllFaceIds()) {
        success = success && settings->getFace(id)->setFromVertex(this->id);
        settings->getFace(id)->setFixed(true);
    }

    return success;
}

} // namespace ms 