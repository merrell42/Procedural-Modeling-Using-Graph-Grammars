#pragma once
#include "model.h"
#include "vertex.h"
#include "face.h"

class Model;
class Vertex;
class Face;

namespace ms {
	class Endpoint {
	public:
		Endpoint(Model* model, int id, int vertexId, int faceId);
		Endpoint* copy();
		Vertex* getVertex() const;
		Face* getFace() const;
		Vec3 getPosition() const;

	private:
		int id;
		int vertexId;
		int faceId;
		Model* model;
	};
}
