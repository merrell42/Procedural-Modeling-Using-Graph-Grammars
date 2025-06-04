#pragma once
#include "model.h"
#include "vertex.h"
#include "face.h"
#include "edge.h"
#include "../primitives/edge_type.h"

namespace ms {

class Model;
class EdgeType3D;
class Vertex;
class Face;
class Edge;

class HalfEdge {
public:
	HalfEdge(Model* model, int id, bool isAtStart, EdgeType3D*, Vec3 dir, int vertexId, int faceId, int edgeId, bool createFace, int faceIndex);
	HalfEdge* copy();
	bool getIsAtStart() const { return isAtStart; }
	int getId() const { return id; };
	EdgeType3D* getEdgeType() const;
	Vec3 getDir() const { return dir; }
	Vertex* getVertex() const;
	Edge* getEdge() const;
	Face* getFace() const;
	Vec3 getPosition() const;
	HalfEdge* next() const;
	HalfEdge* prev() const;
	HalfEdge* twin() const;
	void setEdge(Edge* edge);
	void setFace(Face* face);
	void mergeFaces(HalfEdge* next);
	FaceType3D* getFaceType();
	void transfer(Edge* replacement);
	void maybeMergeNextFace();
	void destroy();

private:
	int id;
	bool isAtStart;
	EdgeType3D* edgeType;
	FaceType3D* faceTypeCached;
	int faceIndex;
	Vec3 dir;
	int vertexId;
	int faceId;
	int edgeId;
	Model* model;
};

}
