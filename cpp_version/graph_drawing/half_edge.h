#pragma once
#include "model.h"
#include "vertex.h"
#include "face.h"
#include "edge.h"
#include "../primitives/edge_type.h"



class Model;
class EdgeType;
class Vertex;
class Face;
class Edge;

class HalfEdge {
public:
	HalfEdge(Model* model, int id, bool isAtStart, EdgeType*, Vec3 dir, int vertexId, int faceId, int edgeId, bool createFace, int faceIndex);
	HalfEdge* copy();
	bool getIsAtStart() const;
	int getId() const;
	EdgeType* getEdgeType() const;
	Vec3 getDir() const;
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
	FaceType* getFaceType();
	void transfer(Edge* replacement);
	void maybeMergeNextFace();
	void destroy();

private:
	int id;
	bool isAtStart;
	EdgeType* edgeType;
	FaceType* faceTypeCached;
	int faceIndex;
	Vec3 dir;
	int vertexId;
	int faceId;
	int edgeId;
	Model* model;
};
