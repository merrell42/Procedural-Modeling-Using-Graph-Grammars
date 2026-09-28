#include "pch.h"
#include "model.h"
#include "face_subdivision.h"
#include "../util/timer.h"

Model::Model() {
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
}

Model::~Model() {
	delete prev;
	delete current;
}

void Model::reset() {
	delete prev;
	delete current;
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
	subdivider.reset();
}

MeshCpp Model::exportMesh(bool deform) {
	if (!subdivider) {
		subdivider = std::make_unique<FaceSubdivider>();
	}
	return current->exportMesh(*subdivider, deform);
}

void Model::accept() {
	GraphDrawing* temp = prev;
	prev = current;
	timer->start("Copy Drawing");
	copyToCurrent();
	timer->stop("Copy Drawing");
	delete temp;
}

void Model::reject() {
	delete current;
	timer->start("Copy Drawing");
	copyToCurrent();
	timer->stop("Copy Drawing");
}

void Model::copyToCurrent() {
	current = new GraphDrawing();
	prev->copyToCurrent();
	current->setBspRootId(prev->getBspRootId());
}

int Model::newId() {
	return idCounter++;
}

GraphDrawing* Model::getCurrent() {
	return current;
}

GraphDrawing* Model::getPrev() {
	return prev;
}
