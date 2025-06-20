#include "pch.h"
#include "model.h"

Model::Model() {
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
}

void Model::reset() {
	delete prev;
	delete current;
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
}

void Model::accept() {
	GraphDrawing* temp = prev;
	prev = current;
	copyToCurrent();
	delete temp;
}

void Model::reject() {
	delete current;
	copyToCurrent();
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
