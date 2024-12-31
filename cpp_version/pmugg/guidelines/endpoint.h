#pragma once
#include <vector>
#include "../node/node.h"
#include "../shape/vec2.h"
#include "../shapes3D/face_type3d.h"
#include "../guidelines/line.h"
#include "../guidelines/line_segment.h"
#include "../guidelines/face_connection.h"
#include "face.h"

namespace ms {

class Endpoint {
public:
    Endpoint(Stats* stats, bool isAtStart, EdgeType* edgeType, float angle, 
             Vec2* dir, float scale, bool createFace, int faceIndex);
    ~Endpoint();

    // Getters
    Node* getNode() const;
    bool isRigid() const;
    Line* getLine() const;
    Face* getFace() const;
    float getAngle() const;
    Vec2 getDir() const;
    std::vector<FaceConnection*> getConnections() const;
    bool getIsAtStart() const;
    EdgeType* getEdgeType() const;
    Vertex* getVertex() const;
    VertexState* getVertexState() const;
    Vec2 getPosition() const;
    LineState* getLineState() const;
    LineSegment* getSegment() const;
    Endpoint* getTwin() const;
    float desiredLength() const;
    Vec2 idealOffset() const;
    Vec2 randomOffset() const;
    FaceType3D* faceType();

    // Setters
    void setAngle(float angle);
    void move(const Vec2& newPosition);

    // Navigation
    Endpoint* next3D();
    Endpoint* next();
    Endpoint* prev();
    Endpoint* clockwise();
    Endpoint* counter();
    Face* nextFace();
    Endpoint* nextOnFace(FaceType3D* faceType);

    // Face operations
    float angleOffset();
    void mergeFaces(Endpoint* next);
    void maybeMergeNextFace();
    void maybeMergePrevFace();
    void transfer(Endpoint* replacement);
    void checkSupport();
    bool hasRightBound();
    int spliceIndex(FaceConnection* connectionA);

    // Drawing
    void drawFace(std::function<void(const Vec2&)> drawPointFunc);
    void fillRenderPositions(std::vector<float>& positions, int dims);
    /*void fillHighlight(RenderData* renderData);
    void draw(Context* context, std::function<Vec2(const Vec2&)> convertToScreen, bool highlighted);
    void draw3D(View3D* view, std::function<Vec2(const Vec2&)> convertToScreen, bool highlighted);
    void highlight(Context* context, std::function<Vec2(const Vec2&)> convertToScreen);
    */
    void print();

    static constexpr float DEVIATION = 0.025f;
    static constexpr float lineThickness = 0.1f;
    // static void drawLine(const Vec2& p0, const Vec2& p1, const std::string& color, RenderData* renderData);
    // static void drawArrow(Context* context, const Vec2& center, float angle, const std::string& color);

private:
    bool isAtStart;
    EdgeType* edgeType;
    Vec2* dir;
    float scale;
    int faceIndex;
    Node* node;
    FaceType3D* faceTypeCached;
    std::map<int, Endpoint*> nextOnFaceCached;
};

} // namespace ms 