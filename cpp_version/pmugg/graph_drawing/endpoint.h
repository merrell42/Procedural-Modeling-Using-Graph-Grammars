#pragma once
#include "model.h"
#include "vertex.h"
#include "face.h"
#include "line.h"
#include "../shapes3D/edge_type3d.h"

namespace ms {

class Model;
class EdgeType3D;
class Vertex;
class Face;
class Line;

class Endpoint {
public:
	Endpoint(Model* model, int id, bool isAtStart, EdgeType3D*, Vec3 dir, int vertexId, int faceId, int lineId);
	Endpoint* copy();
	bool getIsAtStart() const { return isAtStart; }
	int getId() const { return id; };
	EdgeType3D* getEdgeType() const;
	Vec3 getDir() const { return dir; }
	Vertex* getVertex() const;
	Line* getLine() const;
	Face* getFace() const;
	Vec3 getPosition() const;
	Endpoint* next() const;
	Endpoint* prev() const;

private:
	int id;
	bool isAtStart;
	EdgeType3D* edgeType;
	Vec3 dir;
	int vertexId;
	int faceId;
	int lineId;
	Model* model;
};

}
