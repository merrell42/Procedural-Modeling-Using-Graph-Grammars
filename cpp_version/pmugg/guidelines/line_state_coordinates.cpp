#include "line_state_coordinates.h"
#include "brush.h"
#include "intersector.h"
#include "util.h"
#include <cmath>

namespace ms {

LineStateCoordinates::LineStateCoordinates(const std::vector<AngledEdge>& edges, 
                                         float scale, float length)
    : angledEdges(edges)
    , scale(scale)
    , length(length)
    , dirtyLength(length == 0.0f)
    , dirtyRenderData(true)
    , imageSeeds() {
    
    // Initialize image seeds
    for (int i = 0; i < NUM_IMAGE_SEEDS; i++) {
        imageSeeds.push_back(Util::random(100));
    }

    // Scale positions
    for (auto& edge : angledEdges) {
        edge.position = edge.position * scale;
    }
}

std::vector<AngledEdge> LineStateCoordinates::getAngledEdges() const {
    return angledEdges;
}

float LineStateCoordinates::getScale() const {
    return scale;
}

float LineStateCoordinates::getLength() {
    if (dirtyLength) {
        auto p0 = angledEdges[0].getPosition();
        auto p1 = angledEdges[1].getPosition();
        length = (p1 - p0).length();
        dirtyLength = false;
    }
    return length;
}

float LineStateCoordinates::getAngle() const {
    return angledEdges[0].getAngle();
}

LineStateCoordinates LineStateCoordinates::copy() const {
    std::vector<AngledEdge> newEdges;
    for (const auto& edge : angledEdges) {
        newEdges.push_back(edge.copy());
    }
    return LineStateCoordinates(newEdges, scale, length);
}

Vec2* LineStateCoordinates::intersect(const LineStateCoordinates& other,
                                    float thickness1, float thickness2) const {
    auto options = IntersectorOptions{
        .thickness2 = thickness2,
        .parallelIntersect = true
    };

    return Intersector::intersect(
        angledEdges[0].getPosition(),
        angledEdges[1].getPosition(),
        other.angledEdges[0].getPosition(),
        other.angledEdges[1].getPosition(),
        thickness1,
        options);
}

LineStateCoordinates::RenderData LineStateCoordinates::getRenderData(Brush* brush) {
    if (dirtyRenderData) {
        updateRenderData(brush);
        dirtyRenderData = false;
    }
    return renderData;
}

void LineStateCoordinates::updateRenderData(Brush* brush) {
    static const std::vector<float> DEFAULT_TEXCOORDS = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    auto p0 = angledEdges[0].getPosition();
    auto p3 = angledEdges[1].getPosition();
    auto t0 = angledEdges[0].getT();
    auto t3 = angledEdges[1].getT();
    bool reversed = false;

    if (t3 < t0) {
        reversed = true;
        std::swap(t0, t3);
    }

    auto u = p3 - p0;
    auto v0 = Vec2::unitVec(angledEdges[0].getAngle() - M_PI / 2);
    auto v3 = Vec2::unitVec(angledEdges[1].getAngle() + M_PI / 2);

    float length = u.length();
    int numTiles = 1;
    bool tiled = brush ? brush->getTiled() : true;

    if (tiled && brush) {
        float lengthInTiles = length / brush->getTileLength();
        numTiles = std::max(1, static_cast<int>(std::round(lengthInTiles)));
    }

    float offset = brush ? brush->getOffset() : 0.0f;
    float width = tiled ?
        (brush ? brush->getWidth() : DEFAULT_WIDTH) :
        length; // TODO: Use image aspect ratio for non-tiled

    float s0 = -offset - width / 2;
    float s1 = -offset + width / 2;

    auto p00 = p0 + v0 * s0;
    auto p01 = p0 + v0 * s1;
    auto p30 = p3 + v3 * s0;
    auto p31 = p3 + v3 * s1;

    renderData.texcoords.clear();
    renderData.positions.clear();

    for (int i = 0; i < numTiles; i++) {
        // Add texture coordinates
        for (float coord : DEFAULT_TEXCOORDS) {
            renderData.texcoords.push_back(coord);
        }

        // Add positions
        float s0 = static_cast<float>(i) / numTiles;
        float s1 = static_cast<float>(i + 1) / numTiles;

        auto q00 = Vec2::lerp(p00, p30, s0);
        auto q01 = Vec2::lerp(p01, p31, s0);
        auto q10 = Vec2::lerp(p00, p30, s1);
        auto q11 = Vec2::lerp(p01, p31, s1);

        std::vector<float> newPositions = {
            q00.x, q00.y,
            q01.x, q01.y,
            q10.x, q10.y,
            q10.x, q10.y,
            q01.x, q01.y,
            q11.x, q11.y
        };

        renderData.positions.insert(renderData.positions.end(), 
                                  newPositions.begin(), 
                                  newPositions.end());
    }
}

int LineStateCoordinates::getTileImage(const std::vector<void*>& images,
                                     const TileSeeds& tileSeeds,
                                     int tileNum) {
    return Util::consistentRandom(images.size(), 
                                tileSeeds.base + tileNum * tileSeeds.increment);
}

} // namespace ms 