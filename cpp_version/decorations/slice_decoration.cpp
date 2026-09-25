#include "pch.h"
#include "slice_decoration.h"
#include "pending_decoration.h"
#include "../graph_drawing/face.h"
#include "../geometry/matrix4.h"
#include "../geometry/plane.h"
#include "../primitives/face_type.h"
#include <cmath>
#include <utility>

namespace {

Vec3 cutDirection(const Face& face, const Vec3& direction) {
    Vec3 along = face.getFaceType()->getNormal().cross(direction);
    if (along.length2() == 0.0) {
        Vec3 side, up;
        Vec3::orthonormalBasis(direction, side, up);
        return side;
    }
    along.normalize();
    return along;
}

void orderEndpoints(Vec3& start, Vec3& end, const Vec3& along) {
    if (along.dot(start) > along.dot(end)) {
        swap(start, end);
    }
}

} // namespace

SliceDecoration::SliceDecoration(
    const Vec3& direction,
    double spacing,
    EdgeDecoration* edge,
    VertexDecoration* startVertex,
    VertexDecoration* endVertex
) : direction(direction), spacing(spacing), edge(edge), startVertex(startVertex), endVertex(endVertex) {
    this->direction.normalize();
}

void SliceDecoration::resolveChildren(const Decorations& decorations) {
    vector<EdgeDecoration*> edges = { edge };
    resolvePendingChildren<EdgeDecoration, const Vec3&, const Vec3&>(edges, decorations);
    edge = edges[0];

    vector<VertexDecoration*> vertices = { startVertex, endVertex };
    resolvePendingChildren<VertexDecoration, const Matrix4&>(vertices, decorations);
    startVertex = vertices[0];
    endVertex = vertices[1];
}

DecorationOutput SliceDecoration::getOutput(const Face& face) const {
    if (face.isHole() || spacing <= 0.0 || direction.length2() == 0.0) {
        return {};
    }

    Range bounds = face.dirBounds(direction);
    double length = bounds.getHigh() - bounds.getLow();
    int count = static_cast<int>(floor(length / spacing + 1e-9));
    if (count < 1) {
        return {};
    }

    double margin = (length - (count - 1) * spacing) / 2.0;
    Vec3 cutDir = cutDirection(face, direction);
    DecorationOutput output;
    for (int i = 0; i < count; i++) {
        double offset = bounds.getLow() + margin + i * spacing;
        Plane plane(direction, offset);
        vector<Vec3> intersections = face.getIntersections(&plane);
        for (size_t n = 0; n + 1 < intersections.size(); n += 2) {
            Vec3 start = intersections[n];
            Vec3 end = intersections[n + 1];
            orderEndpoints(start, end, cutDir);
            if (edge) {
                output.append(edge->getOutput(start, end));
            }
            if (startVertex) {
                output.append(startVertex->getOutput(Matrix4::translation(start)));
            }
            if (endVertex) {
                output.append(endVertex->getOutput(Matrix4::translation(end)));
            }
        }
    }
    return output;
}
