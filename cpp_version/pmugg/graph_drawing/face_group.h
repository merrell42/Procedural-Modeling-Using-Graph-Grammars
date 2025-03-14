#pragma once
#include "../shape/vec2.h"
#include <vector>
#include "face.h"

namespace ms {

class Face;

class FaceGroup {
public:
    FaceGroup() : faces() {}
    FaceGroup(const std::vector<Face*>& newFaces) : faces(newFaces) {}

    void addFace(Face* face) {
        faces.push_back(face);
    }

    const std::vector<Face*>& getFaces() const {
        return faces;
    }

private:
    std::vector<Face*> faces; // List of pointers to faces in the group
};

} // namespace ms
