#pragma once
#include "../geometry/vec3.h"
#include "../geometry/plane.h"
#include "../primitives/face_type.h"
#include "../util/range.h"

namespace ms {

class Model;
class HalfEdge;
class FaceType;
class FaceGroup;
class Plane;

class Face {
	public:
		Face(Model* model, int id, FaceType* faceType, vector<int> halfedgeIds, bool looped, vector<int> bspNodeIds, int groupId, bool hole);
		Face(Model* model, int id, FaceType* faceType, vector<int> halfedgeIds, vector<int> bspNodeIds);
		Face* copy();
		~Face();
		int getId() const { return id; };
		vector<HalfEdge*> getHalfEdges() const;
		HalfEdge* getHalfEdge(int index) const;
		vector<Vec3> getPositions() const;
		vector<Vec2> getPositions2D() const;
		FaceType* getFaceType() const { return faceType; }
		Range dirBounds(const Vec3& dir) const;
		void append(Face* faceB);
		void setGroup(FaceGroup* group);
		void setLooped(bool looped_) { looped = looped_; }
		FaceGroup* getGroup() const;
		void split(HalfEdge* halfedge);
		bool isHole() { return hole; }
		void destroy();
		void insert(HalfEdge* halfedge, HalfEdge* prevHalfEdge);
		void removeHalfEdge(HalfEdge* halfedge);
		double signedArea();
		void exportMesh(
			vector<Vec3>& positions,
			vector<Vec3>& normals,
			vector<int>& triangles,
			vector<int>& faceIndices
		);
		void addBspNodeId(int bspNodeId) { bspNodeIds.push_back(bspNodeId); }
		const vector<int>& getBspNodeIds() const { return bspNodeIds; } // TODO: Check if this is needed.
		Model* getModel() const { return model; }
		bool addToBsp();
		void removeFromBsp();
		Plane getPlane() const;
		vector<Vec3> getIntersections(Plane* plane);
		bool containsPoint(Vec3 point);
		void createGroup();
		void splitGroup();
		void setHole(bool newHole) { hole = newHole; }
		bool isHole() const { return hole; }

	private:
		int id;
		vector<int> halfedgeIds;
		bool looped;
		FaceType* faceType;
		vector<int> bspNodeIds;
		int groupId;
		bool hole;

		Model* model;

		vector<int> getTriangleIndices();
	};
}
