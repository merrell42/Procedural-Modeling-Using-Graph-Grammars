#pragma once
#include "../pmugg dll/pch.h"
#include "graph_drawing.h"

class GraphDrawing;

namespace ms {
	class Model {
		public:
			Model();
			// Accept the proposed graph drawing.
			void accept();
			// Reject the proposed graph drawing.
			void reject();
			// Reset to an empty graph drawing.
			void reset();
			GraphDrawing* getCurrent();
			int newId();
			// The number of steps we have completed.
			int numSteps;

		private:
			// The currently proposed graph drawing.
			GraphDrawing* current;
			// The saved graph drawing.
			GraphDrawing* prev;
			int idCounter;
	};
}