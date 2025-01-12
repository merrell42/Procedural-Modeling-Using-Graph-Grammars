#pragma once
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
			GraphDrawing* getCurrent() { return current; }
			int newId();

		private:
			// The currently proposed graph drawing.
			GraphDrawing* current;
			// The saved graph drawing.
			GraphDrawing* prev;
			int idCounter;
	};
}