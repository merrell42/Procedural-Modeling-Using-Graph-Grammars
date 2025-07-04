#pragma once
#include "../geometry/vec3.h"
#include "../geometry/plane.h"
#include "../primitives/face_type.h"
#include "../util/range.h"
#include "../memory_counter.h"

class Model;
class HalfEdge;
class FaceType;
class FaceGroup;
class Plane;

class Face {
	public:
		Face(Model* model, int id, FaceType* faceType, vector<int> halfEdgeIds, bool looped, vector<int> bspNodeIds, int groupId, bool hole);
		Face(Model* model, int id, FaceType* faceType, vector<int> halfEdgeIds, vector<int> bspNodeIds);
		Face* copy();
		~Face();
		int getId() const { return id; };
		vector<HalfEdge*> getHalfEdges() const;
		HalfEdge* getHalfEdge(int index) const;
		vector<Vec3> getPositions() const;
		vector<Vec2> getPositions2D() const;
		FaceType* getFaceType() const { return faceType; }
		FaceGroup* getGroup() const;
		bool isHole() const { return hole; }
		void setHole(bool newHole);
		bool isLooped() const { return looped; }
		Model* getModel() const { return model; }
		Plane getPlane() const;
		// Find the bounds of the face along a particular direction.
		Range dirBounds(const Vec3& dir) const;

		void setGroup(FaceGroup* group);
		void setLooped(bool newLooped);
		void insert(HalfEdge* halfEdge, HalfEdge* prevHalfEdge);
		void removeHalfEdge(HalfEdge* halfEdge);
		void addBspNodeId(int bspNodeId);
		bool addToBsp();
		void removeFromBsp();
		double signedArea() const;
		bool containsPoint(Vec3 point) const;
		vector<Vec3> getIntersections(Plane* plane) const;
		void exportMesh(
			vector<Vec3>& positions,
			vector<Vec3>& normals,
			vector<int>& triangles,
			vector<int>& faceIndices
		) const;

		// Add a face to the end of this face.
		void append(Face* faceB);
		void split(HalfEdge* halfEdge);
		void destroy();
		void createGroup();
		void splitGroup();

		bool isAbovePlane(Plane* plane) const;
		bool isBelowPlane(Plane* plane) const;

	private:
		Model* model;
		int id;
		vector<int> halfEdgeIds;
		bool looped;
		FaceType* faceType;
		vector<int> bspNodeIds;
		// A face group represening an outer face with holes.
		int groupId;
		bool hole;

		vector<int> getTriangleIndices() const;
};
