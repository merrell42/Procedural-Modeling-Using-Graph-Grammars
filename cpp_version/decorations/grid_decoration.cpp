#include "pch.h"
#include "grid_decoration.h"
#include "pending_decoration.h"
#include "../graph_drawing/face.h"
#include "../geometry/matrix4.h"
#include "../primitives/face_type.h"
#include "../util/util.h"

GridDecoration::GridDecoration(double spacing) : spacing(spacing) {}

void GridDecoration::setChild(VertexDecoration* child) {
    this->child = child;
}

void GridDecoration::resolveChildren(const Decorations& decorations) {
    vector<VertexDecoration*> children = { child };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(children, decorations);
    child = children[0];
}

DecorationOutput GridDecoration::getOutput(const Face& face) const {
    if (!child || spacing <= 0.0 || face.isHole()) {
        return {};
    }

    const vector<Vec3> positions = face.getPositions();
    if (positions.empty()) {
        return {};
    }

    const FaceType* faceType = face.getFaceType();
    const Vec3& u = faceType->getU();
    const Vec3& v = faceType->getV();
    Vec3 normal = faceType->getNormal();
    normal.normalize();
    double plane = normal.dot(positions[0]);

    Range uBounds = face.dirBounds(u);
    Range vBounds = face.dirBounds(v);
    vector<double> uCoordinates = Util::evenlySpaced(uBounds.getLow(), uBounds.getHigh(), spacing);
    vector<double> vCoordinates = Util::evenlySpaced(vBounds.getLow(), vBounds.getHigh(), spacing);

    DecorationOutput output;
    for (double su : uCoordinates) {
        for (double sv : vCoordinates) {
            Vec3 position = u * su + v * sv + normal * plane;
            if (face.containsPoint(position)) {
                output.append(child->getOutput(Matrix4::translation(position)));
            }
        }
    }
    return output;
}
