#include "pch.h"
#include "model.h"

Model::Model() {
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
}

void Model::reset() {
	delete current;
	delete prev;
	current = new GraphDrawing();
	prev = new GraphDrawing();
	numSteps = 0;
	idCounter = 0;
}

void Model::accept() {
	prev = current;
	current = new GraphDrawing();
	// The values are copied into current through the constructor of each item.
	prev->copy();
	current->setBspRootId(prev->getBspRootId());
}

void Model::reject() {
	current = new GraphDrawing();
	prev->copy();
	current->setBspRootId(prev->getBspRootId());
}

int Model::newId() {
	return idCounter++;
}

GraphDrawing* Model::getCurrent() {
	return current;
}
