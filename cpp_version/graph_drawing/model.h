#pragma once
#include "../pmugg dll/pch.h"
#include "graph_drawing.h"

class GraphDrawing;

// A model contains two graph drawings.
// The current graph drawing is the one that is being edited.
// The previous graph drawing is saved, so that it can be restored.
class Model {
	public:
		Model();
		// Save the current graph drawing.
		void accept();
		// Restore the previous graph drawing.
		void reject();
		// Reset the model to an empty graph drawing.
		void reset();

		GraphDrawing* getCurrent();
		// Returns the snapshot of the previous accepted graph drawing — used
		// by the optimizer's cost machinery to diff edge lengths against the
		// state before the current step's mutation.
		GraphDrawing* getPrev();
		int newId();
		int numSteps;

	private:
		GraphDrawing* current;
		GraphDrawing* prev;
		int idCounter;

		void copyToCurrent();
};