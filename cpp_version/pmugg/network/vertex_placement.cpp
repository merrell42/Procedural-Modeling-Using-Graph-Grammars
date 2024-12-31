

void VertexPlacement::addConstraint() {
    if (freeFaceIds.empty()) {
        throw std::runtime_error("No free faces available for constraint");
    }
    int freeFaceId = freeFaceIds[0];
    settings->getFace(freeFaceId)->constrain(true);
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

void VertexPlacement::propagate(int id) {
    if (unfreeFaceIds.size() >= 3) {
        settings->addToOrder(this->id, "vertex");
        
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
        for (auto* endpoint : vertex->getEndpoints()) {
            int edgeId = endpoint->getLine()->getNode()->getId();
            auto* ePlace = settings->getEdge(edgeId);
            if (ePlace) {
                ePlace->addConstraint(this->id);
            }

            auto* prev = endpoint->prev();
            if (prev->getEdgeType()->faceData.size() == 1) {
                // If prev edge only has one endpoint, propagate to previous vertex
                int prevEdgeId = prev->getLine()->getNode()->getId();
                auto* prevEPlace = settings->getEdge(prevEdgeId);
                if (prevEPlace) {
                    prevEPlace->addConstraint(this->id);
                }
            }
        }
    }
}

Matrix3* VertexPlacement::getA(const std::vector<int>& faceIds) {
    if (faceIds.size() != 3) {
        return nullptr;
    }

    std::vector<Vec3> normals;
    for (int id : faceIds) {
        auto* fPlace = settings->getFace(id);
        normals.push_back(fPlace->getNormal());
    }

    auto* A = new Matrix3();
    for (int i = 0; i < 3; i++) {
        A->setRow(i, normals[i]);
    }

    if (std::abs(A->determinant()) < 1e-8) {
        delete A;
        return nullptr;
    }

    return A;
}

Matrix3* VertexPlacement::getM() {
    if (!M) {
        auto* A = getA(unfreeFaceIds);
        if (!A) {
            return nullptr;
        }
        M = A->inverse();
        if (!M) {
            delete A;
        }
    }
    return M;
}

void VertexPlacement::setPosition() {
    std::vector<float> D;
    for (int i = 0; i < 3; i++) {
        auto* fPlace = settings->getFace(unfreeFaceIds[i]);
        D.push_back(fPlace->getD());
    }

    auto* M = getM();
    if (!M) {
        throw std::runtime_error("Failed to get transformation matrix");
    }

    Vec3 result = *M * Vec3(D[0], D[1], D[2]);
    slope = Vec3::ORIGIN;
    value = result;
}

Range VertexPlacement::getRange() {
    std::vector<float> m3;
    std::vector<float> b3;

    // Get the change in position for each face
    for (int i = 0; i < 3; i++) {
        auto* fPlace = settings->getFace(unfreeFaceIds[i]);
        auto mb = fPlace->getChangeMB();
        m3.push_back(mb.m);
        b3.push_back(mb.b);
    }

    auto* M = getM();
    if (!M) {
        throw std::runtime_error("Failed to get transformation matrix");
    }

    Vec3 m = *M * Vec3(m3[0], m3[1], m3[2]);
    Vec3 b = *M * Vec3(b3[0], b3[1], b3[2]);

    slope = m;
    value = b;

    Range range(-INFINITY, INFINITY);
    for (int i = 0; i < 3; i++) {
        Range componentRange(settings->lower[i], settings->upper[i]);
        range = range.intersect(Range::transformCreate(m[i], b[i], componentRange));
    }

    return range;
}

VertexPlacement::ChangeMB VertexPlacement::getChangeMB() const {
    return {slope, value};
}

} // namespace ms 