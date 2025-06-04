#pragma once
#include "../graph_drawing/edge.h"
#include "../graph_drawing/face.h"
#include "../geometry/plane.h"

namespace ms {
	class Model;

	enum class PlaneClassification {
		BELOW = -1,
		BOTH = 0,
		ABOVE = 1,
		ON_PLANE = 2
	};

	class BspNode {
		public:
			BspNode(Model* model, int id);
			BspNode(Model* model, int id, int parentId, int aboveId, int belowId, Plane* plane, std::vector<int> faceIds, std::vector<int> lineIds);
			~BspNode() { delete plane; }
			BspNode* copy();
			void setParentId(int newParentId) { parentId = newParentId; }
			bool addLine(Line* line);
			bool addFace(Face* face);
			void removeLine(Line* line);
			void removeFace(Face* face);
			int getId() { return id; }

		private:
			Model* model;
			int id;
			int parentId;
			int aboveId;
			int belowId;
			std::vector<int> faceIds;
			std::vector<int> lineIds;
			Plane* plane;

			void connectAbove(BspNode* above);
			void connectBelow(BspNode* below);
			void connectLine(Line* line);
			void connectFace(Face* face);
			BspNode* getAboveNode();
			BspNode* getBelowNode();
			bool hasLineIntersection(Line* line);
			bool hasFaceIntersection(Face* face);

			PlaneClassification classifyLine(Line* line);
			PlaneClassification classifyFace(Face* face);
			Plane* getPlane();
			bool isPointAbovePlane(Vec3 point);
			bool isPointBelowPlane(Vec3 point);
			void deleteIfEmpty();
	};
}