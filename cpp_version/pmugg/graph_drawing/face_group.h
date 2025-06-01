#pragma once
#include "../shape/vec2.h"
#include <vector>
#include "face.h"
#include "model.h"

namespace ms {

class Model;
class Face;

// A face group represent an outer face with holes poked into it.
// The first face in the group is the outer face. The other faces are holes.
class FaceGroup {
public:
    FaceGroup(Model* model, int id);
    FaceGroup(Model* model, int id, std::vector<int> faceIds);
    FaceGroup* copy();
    int getId() const { return id; };
    void addFace(Face* face);
    void insertFace(Face* face, int index);
    void removeFace(Face* face);
    std::vector<Face*> getFaces() const;
    void destroyIfEmpty();
    void connectHole(FaceGroup* groupB);

private:
    int id;
    std::vector<int> faceIds;
    
	Model* model;
};

} // namespace ms
