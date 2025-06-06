#pragma once
#include "../pmugg dll/pch.h"
#include "graph_drawing.h"

class GraphDrawing;

class Model {
	public:
		Model();
		void accept();
		void reject();
		void reset();
		GraphDrawing* getCurrent();
		int newId();
		int numSteps;

	private:
		GraphDrawing* current;
		GraphDrawing* prev;
		int idCounter;
};