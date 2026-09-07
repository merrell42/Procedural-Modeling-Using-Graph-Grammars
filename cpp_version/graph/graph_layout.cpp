#include "pch.h"
#include "graph_layout.h"
#include "graph.h"
#include "graph_edge.h"
#include "graph_half_edge.h"
#include "graph_vertex.h"
#include "../geometry/mesh.h"
#include "../primitives/edge_type.h"
#include "../graph/graph_face.h"
#include "../primitives/face_type.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

constexpr double STANDARD_LENGTH = 1.0;
constexpr double LEAF_LENGTH = 0.5;
constexpr double COMPONENT_SPACING = 2.5;
constexpr double LAYOUT_MARGIN = 0.05;
constexpr double FACE_RECT_WIDTH = 0.18;
constexpr double EDGE_LINE_HALF_WIDTH = 0.006;
constexpr double EDGE_LINE_LIFT = 0.004;
constexpr double FACE_ONLY_HALF_WIDTH = 0.4;
constexpr double FACE_ONLY_HALF_HEIGHT = 0.4;

struct ColorKey {
    float red;
    float green;
    float blue;

    bool operator<(const ColorKey& other) const {
        if (red != other.red) return red < other.red;
        if (green != other.green) return green < other.green;
        return blue < other.blue;
    }
};

int countConnections(const GraphVertex* vertex) {
    int count = 0;
    for (auto* halfEdge : vertex->getHalfEdges()) {
        if (halfEdge && halfEdge->getEdge()) {
            count++;
        }
    }
    return count;
}

GraphHalfEdge* getTwin(GraphHalfEdge* halfEdge) {
    if (!halfEdge || !halfEdge->getEdge()) {
        return nullptr;
    }
    try {
        return halfEdge->getTwin();
    } catch (...) {
        return nullptr;
    }
}

Vec3 pickPerpendicular(const Vec3& axis) {
    Vec3 reference = std::abs(axis.getX()) < 0.9 ? Vec3(1, 0, 0, false) : Vec3(0, 1, 0, false);
    Vec3 perpendicular = axis.cross(reference);
    if (perpendicular.length2() < 1e-12) {
        perpendicular = axis.cross(Vec3(0, 0, 1, false));
    }
    perpendicular.normalize();
    return perpendicular;
}

void facePlaneBasis(const Vec3& normal, Vec3& outU, Vec3& outV) {
    Vec3 reference = Vec3::X_AXIS;
    if (std::abs(normal.dot(reference)) > 0.9) {
        reference = Vec3::Y_AXIS;
    }
    outV = normal.cross(reference);
    outU = outV.cross(normal);
    outU.normalize();
    outV.normalize();
}

bool isFaceOnlyGraph(const Graph* graph) {
    if (!graph) {
        return false;
    }
    return graph->getEdges().empty()
        && graph->getHalfEdges().empty()
        && graph->getVertices().empty()
        && !graph->getFaces().empty();
}

void addQuadWithNormal(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d,
    const Vec3& normal
) {
    const int baseIndex = (int)positions.size();
    positions.push_back(a);
    positions.push_back(b);
    positions.push_back(c);
    positions.push_back(d);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
    triangles.push_back(baseIndex);
    triangles.push_back(baseIndex + 1);
    triangles.push_back(baseIndex + 2);
    triangles.push_back(baseIndex);
    triangles.push_back(baseIndex + 2);
    triangles.push_back(baseIndex + 3);
}

void addQuadWithFaceNormal(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d,
    const Vec3& faceNormal
) {
    const Vec3 geometricNormal = (b - a).cross(c - a);
    if (geometricNormal.dot(faceNormal) > 0.0) {
        addQuadWithNormal(positions, normals, triangles, a, d, c, b, faceNormal);
    } else {
        addQuadWithNormal(positions, normals, triangles, a, b, c, d, faceNormal);
    }
}

void addPlanarRectangle(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& center,
    const Vec3& axisU,
    const Vec3& axisV,
    double halfWidth,
    double halfHeight,
    const Vec3& normal
) {
    const Vec3 a = center - axisU * halfWidth - axisV * halfHeight;
    const Vec3 b = center + axisU * halfWidth - axisV * halfHeight;
    const Vec3 c = center + axisU * halfWidth + axisV * halfHeight;
    const Vec3 d = center - axisU * halfWidth + axisV * halfHeight;

    addQuadWithFaceNormal(positions, normals, triangles, a, b, c, d, normal);
}

void addTriangle(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c
) {
    Vec3 normal = (b - a).cross(c - a);
    if (normal.length2() > 1e-12) {
        normal = normal.normalize();
    } else {
        normal = Vec3(0, 1, 0, false);
    }

    int baseIndex = (int)positions.size();
    positions.push_back(a);
    positions.push_back(b);
    positions.push_back(c);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
    triangles.push_back(baseIndex);
    triangles.push_back(baseIndex + 1);
    triangles.push_back(baseIndex + 2);
}

void addQuad(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d
) {
    addTriangle(positions, normals, triangles, a, b, c);
    addTriangle(positions, normals, triangles, a, c, d);
}

ColorKey colorFromFaceType(const FaceType* faceType) {
    if (!faceType) {
        return {0.11f, 0.24f, 0.42f};
    }
    const Vec3 color = faceType->getColor();
    return {(float)color.getX(), (float)color.getY(), (float)color.getZ()};
}

Vec3 normalizeOrZero(const Vec3& v, const Vec3& fallback) {
    if (v.length2() < 1e-12) {
        return fallback;
    }
    Vec3 result = v;
    result.normalize();
    return result;
}

GraphHalfEdge* findHalfForFaceType(
    GraphHalfEdge* halfA,
    GraphHalfEdge* halfB,
    FaceType* faceType
) {
    if (halfA && halfA->getFace() && halfA->getFace()->getType() == faceType) {
        return halfA;
    }
    if (halfB && halfB->getFace() && halfB->getFace()->getType() == faceType) {
        return halfB;
    }
    return nullptr;
}

Vec3 computeRightInPlane(const Vec3& axis) {
    Vec3 right = axis.cross(Vec3(0, 0, 1, false));
    if (right.length2() < 1e-12) {
        right = axis.cross(Vec3(0, 1, 0, false));
    }
    right.normalize();
    return right;
}

Vec3 intoFaceDirectionForHalf(const Vec3& halfDir, const Vec3& faceNormal) {
    const Vec3 normal = normalizeOrZero(faceNormal, Vec3(0, 0, 1, false));
    Vec3 axisU;
    Vec3 axisV;
    facePlaneBasis(normal, axisU, axisV);

    Vec3 tangent = halfDir - normal * normal.dot(halfDir);
    tangent = normalizeOrZero(tangent, axisU);

    // The incident face lies to the left of the directed half-edge in its plane.
    Vec3 intoFace = normal.cross(tangent);
    return normalizeOrZero(intoFace, axisV);
}

Vec3 intoFaceDirectionForFaceData(
    const Vec3& start,
    const Vec3& end,
    const FaceData& faceData,
    GraphHalfEdge* halfA,
    GraphHalfEdge* halfB
) {
    const Vec3 normal = normalizeOrZero(faceData.type->getNormal(), Vec3(0, 0, 1, false));
    Vec3 axisU;
    Vec3 axisV;
    facePlaneBasis(normal, axisU, axisV);

    GraphHalfEdge* half = findHalfForFaceType(halfA, halfB, faceData.type);
    if (half) {
        return intoFaceDirectionForHalf(half->getDir(), faceData.type->getNormal());
    }

    Vec3 layoutEdge = end - start;
    Vec3 tangent = layoutEdge - normal * normal.dot(layoutEdge);
    tangent = normalizeOrZero(tangent, axisU);
    Vec3 leftOfEdge = normal.cross(tangent);
    leftOfEdge = normalizeOrZero(leftOfEdge, axisV);
    return faceData.onRight ? leftOfEdge * -1.0 : leftOfEdge;
}

void addFaceRectangle(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& start,
    const Vec3& end,
    const Vec3& intoFace,
    double width,
    const Vec3& faceNormal
) {
    const Vec3 offset = intoFace * width;
    addQuadWithFaceNormal(
        positions,
        normals,
        triangles,
        start,
        end,
        end + offset,
        start + offset,
        faceNormal
    );
}

void addFaceRectangleForFaceData(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& start,
    const Vec3& end,
    const FaceData& faceData,
    GraphHalfEdge* halfA,
    GraphHalfEdge* halfB,
    double width
) {
    if (!faceData.type) {
        return;
    }

    const Vec3 faceNormal = faceData.type->getNormal();
    const Vec3 intoFace = intoFaceDirectionForFaceData(start, end, faceData, halfA, halfB);
    addFaceRectangle(positions, normals, triangles, start, end, intoFace, width, faceNormal);
}

void addFaceRectangleLayoutFallback(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& start,
    const Vec3& end,
    double width
) {
    Vec3 axis = end - start;
    if (axis.length2() < 1e-12) {
        return;
    }
    axis.normalize();
    const Vec3 layoutNormal = Vec3(0, 0, 1, false);
    const Vec3 intoFace = computeRightInPlane(axis);
    addFaceRectangle(positions, normals, triangles, start, end, intoFace, width, layoutNormal);
}

Vec3 computePlaneNormal(const Vec3& axis, const Vec3& right) {
    Vec3 normal = axis.cross(right);
    if (normal.length2() < 1e-12) {
        return Vec3(0, 0, 1, false);
    }
    return normal.normalize();
}

void addEdgeLine(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& start,
    const Vec3& end,
    const Vec3& widthDir,
    const Vec3& lift
) {
    const Vec3 offset = widthDir * EDGE_LINE_HALF_WIDTH;
    addQuad(
        positions,
        normals,
        triangles,
        start - offset + lift,
        end - offset + lift,
        end + offset + lift,
        start + offset + lift
    );
}

void addThinLine(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    const Vec3& start,
    const Vec3& end,
    double halfWidth
) {
    Vec3 axis = end - start;
    if (axis.length2() < 1e-12) {
        return;
    }
    axis.normalize();
    const Vec3 sideA = pickPerpendicular(axis);
    const Vec3 offset = sideA * halfWidth;
    addQuad(positions, normals, triangles, start - offset, end - offset, end + offset, start + offset);
}

void normalizePositions(std::unordered_map<GraphVertex*, Vec3>& positions) {
    if (positions.empty()) {
        return;
    }

    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double minZ = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    double maxZ = -std::numeric_limits<double>::infinity();

    for (const auto& entry : positions) {
        minX = std::min(minX, entry.second.getX());
        minY = std::min(minY, entry.second.getY());
        minZ = std::min(minZ, entry.second.getZ());
        maxX = std::max(maxX, entry.second.getX());
        maxY = std::max(maxY, entry.second.getY());
        maxZ = std::max(maxZ, entry.second.getZ());
    }

    const double centerX = (minX + maxX) * 0.5;
    const double centerY = (minY + maxY) * 0.5;
    const double centerZ = (minZ + maxZ) * 0.5;
    const double extentX = std::max(maxX - minX, 1e-6);
    const double extentY = std::max(maxY - minY, 1e-6);
    const double extentZ = std::max(maxZ - minZ, 1e-6);
    const double extent = std::max({extentX, extentY, extentZ});
    const double scale = (1.0 - 2.0 * LAYOUT_MARGIN) / extent;

    for (auto& entry : positions) {
        entry.second = Vec3(
            (entry.second.getX() - centerX) * scale,
            (entry.second.getY() - centerY) * scale,
            (entry.second.getZ() - centerZ) * scale,
            false
        );
    }
}

void layoutComponent(
    GraphVertex* startVertex,
    const Vec3& origin,
    std::unordered_map<GraphVertex*, Vec3>& positions
) {
    positions[startVertex] = origin;

    std::queue<GraphHalfEdge*> pendingHalfEdges;
    for (auto* halfEdge : startVertex->getHalfEdges()) {
        if (halfEdge && halfEdge->getEdge()) {
            pendingHalfEdges.push(halfEdge);
        }
    }

    while (!pendingHalfEdges.empty()) {
        GraphHalfEdge* halfEdge = pendingHalfEdges.front();
        pendingHalfEdges.pop();

        GraphVertex* prevVertex = halfEdge->getVertex();
        GraphHalfEdge* twin = getTwin(halfEdge);
        if (!twin) {
            continue;
        }

        GraphVertex* nextVertex = twin->getVertex();
        if (!nextVertex || positions.count(nextVertex) > 0) {
            continue;
        }

        double edgeLength = STANDARD_LENGTH;
        if (countConnections(prevVertex) == 1 || countConnections(nextVertex) == 1) {
            edgeLength = LEAF_LENGTH;
        }

        Vec3 direction = halfEdge->getDir();
        if (direction.length2() < 1e-12) {
            const double angle = halfEdge->getAngle();
            direction = Vec3(std::cos(angle), std::sin(angle), 0.0, false);
        } else {
            direction.normalize();
        }

        positions[nextVertex] = positions[prevVertex] + direction * edgeLength;

        for (auto* nextHalfEdge : nextVertex->getHalfEdges()) {
            if (!nextHalfEdge || !nextHalfEdge->getEdge()) {
                continue;
            }
            if (nextHalfEdge->getEdge() == halfEdge->getEdge()) {
                continue;
            }
            pendingHalfEdges.push(nextHalfEdge);
        }
    }
}

std::unordered_map<GraphVertex*, Vec3> layoutGraphPositions(const Graph* graph) {
    std::unordered_map<GraphVertex*, Vec3> positions;
    if (!graph) {
        return positions;
    }

    int componentIndex = 0;
    for (auto* vertex : graph->getVertices()) {
        if (!vertex || positions.count(vertex) > 0) {
            continue;
        }
        if (countConnections(vertex) == 0) {
            continue;
        }

        const Vec3 componentOrigin(componentIndex * COMPONENT_SPACING, 0.0, 0.0, false);
        layoutComponent(vertex, componentOrigin, positions);
        componentIndex++;
    }

    normalizePositions(positions);
    return positions;
}

MeshCpp createEmptyGraphMesh() {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<int> triangles;

    const double radius = 0.15;
    addThinLine(positions, normals, triangles, Vec3(-radius, -radius, 0, false), Vec3(radius, radius, 0, false), 0.01);
    addThinLine(positions, normals, triangles, Vec3(-radius, radius, 0, false), Vec3(radius, -radius, 0, false), 0.01);

    SubmeshCpp submesh = createSubmesh(positions, normals, triangles, {}, 0.8f, 0.8f, 0.8f);
    MeshCpp mesh{};
    mesh.numSubmeshes = 1;
    mesh.submeshes = new SubmeshCpp[1];
    mesh.submeshes[0] = submesh;
    return mesh;
}

MeshCpp createMeshFromGroups(const std::map<ColorKey, std::tuple<std::vector<Vec3>, std::vector<Vec3>, std::vector<int>>>& groups) {
    MeshCpp mesh{};
    mesh.numSubmeshes = (int)groups.size();
    if (mesh.numSubmeshes == 0) {
        mesh.submeshes = nullptr;
        return mesh;
    }

    mesh.submeshes = new SubmeshCpp[mesh.numSubmeshes];
    int submeshIndex = 0;
    for (const auto& entry : groups) {
        const auto& geometry = entry.second;
        mesh.submeshes[submeshIndex++] = createSubmesh(
            std::get<0>(geometry),
            std::get<1>(geometry),
            std::get<2>(geometry),
            {},
            entry.first.red,
            entry.first.green,
            entry.first.blue
        );
    }
    return mesh;
}

MeshCpp createFaceOnlyGraphMesh(const Graph* graph) {
    std::map<ColorKey, std::tuple<std::vector<Vec3>, std::vector<Vec3>, std::vector<int>>> groups;

    for (auto* face : graph->getFaces()) {
        if (!face || !face->getType()) {
            continue;
        }

        FaceType* faceType = face->getType();
        Vec3 normal = faceType->getNormal();
        if (normal.length2() < 1e-12) {
            normal = Vec3(0, 0, 1, false);
        }

        Vec3 axisU;
        Vec3 axisV;
        facePlaneBasis(normal, axisU, axisV);

        ColorKey color = colorFromFaceType(faceType);
        auto& geometry = groups[color];
        addPlanarRectangle(
            std::get<0>(geometry),
            std::get<1>(geometry),
            std::get<2>(geometry),
            Vec3(0, 0, 0, false),
            axisU,
            axisV,
            FACE_ONLY_HALF_WIDTH,
            FACE_ONLY_HALF_HEIGHT,
            normal
        );
    }

    if (groups.empty()) {
        return createEmptyGraphMesh();
    }

    return createMeshFromGroups(groups);
}

} // namespace

MeshCpp exportGraphMesh(const Graph* graph) {
    if (!graph) {
        return createEmptyGraphMesh();
    }

    if (isFaceOnlyGraph(graph)) {
        return createFaceOnlyGraphMesh(graph);
    }

    if (graph->getEdges().empty()) {
        return createEmptyGraphMesh();
    }

    const auto positions = layoutGraphPositions(graph);
    if (positions.empty()) {
        return createEmptyGraphMesh();
    }

    std::map<ColorKey, std::tuple<std::vector<Vec3>, std::vector<Vec3>, std::vector<int>>> groups;

    for (auto* edge : graph->getEdges()) {
        if (!edge) {
            continue;
        }

        const auto& halfEdgeGroups = edge->getHalfEdges();
        if (halfEdgeGroups.size() < 2 || halfEdgeGroups[0].empty() || halfEdgeGroups[1].empty()) {
            continue;
        }

        GraphHalfEdge* halfA = halfEdgeGroups[0][0];
        GraphHalfEdge* halfB = halfEdgeGroups[1][0];
        if (!halfA || !halfB) {
            continue;
        }

        GraphVertex* vertexA = halfA->getVertex();
        GraphVertex* vertexB = halfB->getVertex();
        if (!vertexA || !vertexB) {
            continue;
        }

        auto posA = positions.find(vertexA);
        auto posB = positions.find(vertexB);
        if (posA == positions.end() || posB == positions.end()) {
            continue;
        }

        const Vec3& start = posA->second;
        const Vec3& end = posB->second;
        Vec3 axis = end - start;
        if (axis.length2() < 1e-12) {
            continue;
        }
        axis.normalize();
        const Vec3 layoutRight = computeRightInPlane(axis);
        const Vec3 planeNormal = computePlaneNormal(axis, layoutRight);
        const Vec3 edgeLift = planeNormal * EDGE_LINE_LIFT;

        if (!edge->getType()) {
            continue;
        }

        const auto& faceData = edge->getType()->getFaceData();
        if (faceData.empty()) {
            ColorKey color = {0.11f, 0.24f, 0.42f};
            auto& geometry = groups[color];
            addFaceRectangleLayoutFallback(
                std::get<0>(geometry),
                std::get<1>(geometry),
                std::get<2>(geometry),
                start,
                end,
                FACE_RECT_WIDTH
            );
            continue;
        }

        for (const FaceData& fd : faceData) {
            ColorKey color = colorFromFaceType(fd.type);
            auto& geometry = groups[color];
            addFaceRectangleForFaceData(
                std::get<0>(geometry),
                std::get<1>(geometry),
                std::get<2>(geometry),
                start,
                end,
                fd,
                halfA,
                halfB,
                FACE_RECT_WIDTH
            );
        }

        ColorKey edgeColor = {0.0f, 0.0f, 0.0f};
        auto& edgeGeometry = groups[edgeColor];
        addEdgeLine(
            std::get<0>(edgeGeometry),
            std::get<1>(edgeGeometry),
            std::get<2>(edgeGeometry),
            start,
            end,
            layoutRight,
            edgeLift
        );
    }

    if (groups.empty()) {
        return createEmptyGraphMesh();
    }

    return createMeshFromGroups(groups);
}
