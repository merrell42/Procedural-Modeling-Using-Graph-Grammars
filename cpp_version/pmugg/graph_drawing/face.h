#pragma once
#include "model.h"
#include "endpoint.h"
#include "../util/range.h"
#include "../shape/vec3.h"

namespace ms {

class Model;
class Endpoint;

class Face {
	public:
		Face(Model* model, int id, std::vector<int> endpointIds, bool looped);
		Face(Model* model, int id, std::vector<int> endpointIds);
		Face* copy();
		int getId() const { return id; };
		std::vector<Endpoint*> getEndpoints() const;
		Endpoint* getEndpoint(int index) const;
		Range dirBounds(const Vec3& dir) const;
		void append(Face* faceB);
		void setLooped(bool looped_) { looped = looped_; }

	private:
		int id;
		std::vector<int> endpointIds;
		bool looped;

		Model* model;
	};
}
