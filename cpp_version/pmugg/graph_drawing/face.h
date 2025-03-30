#pragma once
#include "model.h"
#include "endpoint.h"
#include "../util/range.h"
#include "../shape/vec3.h"
#include "../shapes3D/face_type3d.h"

namespace ms {

class Model;
class Endpoint;
class FaceType3D;
class FaceGroup;

class Face {
	public:
		Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, bool looped);
		Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds);
		Face* copy();
		int getId() const { return id; };
		std::vector<Endpoint*> getEndpoints() const;
		Endpoint* getEndpoint(int index) const;
		std::vector<Vec3> getPositions() const;
		FaceType3D* getFaceType() const { return faceType; }
		Range dirBounds(const Vec3& dir) const;
		void append(Face* faceB);
		void setLooped(bool looped_) { looped = looped_; }
		FaceGroup* getGroup() const;
		void split(Endpoint* endpoint);
		bool isHole() { return false; }
		void destroy();
		void insert(Endpoint* endpoint, Endpoint* prevEndpoint);
		void removeEndpoint(Endpoint* endpoint);

	private:
		int id;
		std::vector<int> endpointIds;
		bool looped;
		FaceType3D* faceType;

		Model* model;
	};
}
