#include "pch.h"
#include "model.h"

namespace ms {

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
}

void Model::accept() {
	prev = current;
	current = new GraphDrawing();
	prev->copy();
}

void Model::reject() {
	current = new GraphDrawing();
	prev->copy();
}

int Model::newId() {
	return idCounter++;
}

}