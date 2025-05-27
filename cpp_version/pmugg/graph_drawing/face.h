#pragma once
#include "../util/range.h"
#include "../shape/vec3.h"
#include "../shapes3D/face_type3d.h"
#include "../bsp/plane.h"

namespace ms {

class Model;
class Endpoint;
class FaceType3D;
class FaceGroup;
class Plane;

class Face {
	public:
		Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, bool looped, std::vector<int> bspNodeIds);
		Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, std::vector<int> bspNodeIds);
		Face* copy();
		~Face();
		int getId() const { return id; };
		std::vector<Endpoint*> getEndpoints() const;
		Endpoint* getEndpoint(int index) const;
		std::vector<Vec3> getPositions() const;
		std::vector<Vec2> getPositions2D() const;
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
		double signedArea();
		void exportMesh(
			std::vector<Vec3>& positions,
			std::vector<Vec3>& normals,
			std::vector<int>& triangles,
			std::vector<int>& faceIndices
		);
		void addBspNodeId(int bspNodeId) { bspNodeIds.push_back(bspNodeId); }
		const std::vector<int>& getBspNodeIds() const { return bspNodeIds; } // TODO: Check if this is needed.
		Model* getModel() const { return model; }
		bool addToBsp();
		void removeFromBsp();
		Plane getPlane() const;
		std::vector<Vec3> getIntersections(Plane* plane);
		bool containsPoint(Vec3 point);

	private:
		int id;
		std::vector<int> endpointIds;
		bool looped;
		FaceType3D* faceType;
		std::vector<int> bspNodeIds;

		Model* model;

		std::vector<int> getTriangleIndices();
	};
}
