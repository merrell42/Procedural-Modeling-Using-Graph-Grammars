#pragma once
#include "../geometry/vec3.h"
#include "../geometry/plane.h"
#include "../primitives/face_type.h"
#include "../util/range.h"

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
		Range dirBounds(const Vec3& dir) const;
		void append(Face* faceB);
		void setGroup(FaceGroup* group);
		void setLooped(bool newLooped);
		FaceGroup* getGroup() const;
		void split(HalfEdge* halfEdge);
		bool isHole() { return hole; }
		void destroy();
		void insert(HalfEdge* halfEdge, HalfEdge* prevHalfEdge);
		void removeHalfEdge(HalfEdge* halfEdge);
		double signedArea();
		void exportMesh(
			vector<Vec3>& positions,
			vector<Vec3>& normals,
			vector<int>& triangles,
			vector<int>& faceIndices
		);
		void addBspNodeId(int bspNodeId);
		const vector<int>& getBspNodeIds() const;
		Model* getModel() const { return model; }
		bool addToBsp();
		void removeFromBsp();
		Plane getPlane() const;
		vector<Vec3> getIntersections(Plane* plane);
		bool containsPoint(Vec3 point);
		void createGroup();
		void splitGroup();
		void setHole(bool newHole);
		bool isHole() const;

	private:
		int id;
		vector<int> halfEdgeIds;
		bool looped;
		FaceType* faceType;
		vector<int> bspNodeIds;
		int groupId;
		bool hole;

		Model* model;

		vector<int> getTriangleIndices();
};
