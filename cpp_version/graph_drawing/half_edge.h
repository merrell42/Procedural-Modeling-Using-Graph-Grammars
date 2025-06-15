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

// A half edge is an edge pointing in a particular direction.
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

	// Get the next and previous half-edge on the face.
	HalfEdge* next() const;
	HalfEdge* prev() const;

	void setEdge(Edge* edge);
	void setFace(Face* face);

	// Merge the face of this half-edge with the face of the next half-edge.
	void mergeFaces(HalfEdge* next);
	FaceType* getFaceType();

	// Transfer the half-edge to a new edge.
	void transfer(Edge* replacement);
	void destroy();

private:
	int id;
	// If the half-edge is at the start position, it is pointed in the direction of the edge type.
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
