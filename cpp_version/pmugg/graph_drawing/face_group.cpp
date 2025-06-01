#include "pch.h"
#include "face_group.h"

namespace ms {

FaceGroup::FaceGroup(Model* model, int id) : model(model), id(id), faceIds() {
    model->getCurrent()->addFaceGroup(id, this);
}

FaceGroup::FaceGroup(Model* model, int id, std::vector<int> faceIds) : model(model), id(id), faceIds(faceIds) {
    model->getCurrent()->addFaceGroup(id, this);
}

FaceGroup* FaceGroup::copy() {
	return new FaceGroup(model, id, faceIds);
}

void FaceGroup::addFace(Face* face) {
    faceIds.push_back(face->getId());
    face->setGroup(this);
}

void FaceGroup::insertFace(Face* face, int index) {
    faceIds.insert(faceIds.begin() + index, face->getId());
    face->setGroup(this);
}

void FaceGroup::removeFace(Face* face) {
    faceIds.erase(std::remove(faceIds.begin(), faceIds.end(), face->getId()), faceIds.end());
}

std::vector<Face*> FaceGroup::getFaces() const {
	std::vector<Face*> result;
	for (int i = 0; i < faceIds.size(); i++) {
		result.push_back(model->getCurrent()->getFace(faceIds[i]));
	}
	return result;
}

void FaceGroup::destroyIfEmpty() {
    if (faceIds.size() == 0) {
        model->getCurrent()->removeFaceGroup(this);
        delete this;
    }
}

} // namespace ms 