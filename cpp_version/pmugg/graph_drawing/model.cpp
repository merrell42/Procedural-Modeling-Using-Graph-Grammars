#include "model.h"


namespace ms {

Model::Model() {
	current = new GraphDrawing();
	prev = new GraphDrawing();
}

void Model::accept() {
	prev = current;
	current = prev->copy();
}

void Model::reject() {
	current = prev->copy();
}

}