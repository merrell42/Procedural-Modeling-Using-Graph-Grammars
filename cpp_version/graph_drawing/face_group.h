#pragma once
#include "../geometry/vec2.h"
#include <vector>
#include "face.h"
#include "model.h"



class Model;
class Face;

// A face group represent an outer face with holes poked into it.
// The first face in the group is the outer face. The other faces are holes.
class FaceGroup {
public:
    FaceGroup(Model* model, int id);
    FaceGroup(Model* model, int id, vector<int> faceIds);
    FaceGroup* copy();

    int getId() const { return id; };
    void addFace(Face* face);
    void insertFace(Face* face, int index);
    void removeFace(Face* face);
    vector<Face*> getFaces() const;
    void destroyIfEmpty();
    void connectHole(FaceGroup* groupB);

private:
    int id;
    vector<int> faceIds;
    
	Model* model;
};
