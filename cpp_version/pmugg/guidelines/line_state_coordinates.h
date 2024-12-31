#pragma once
#include "../shape/vec2.h"
#include <vector>
#include <memory>

namespace ms {

class Brush;

struct AngledEdge {
    Vec2 position;
    float angle;
    float t;

    AngledEdge(const Vec2& pos, float ang, float t = 0.0f)
        : position(pos), angle(ang), t(t) {}

    AngledEdge copy() const {
        return AngledEdge(position, angle, t);
    }

    Vec2 getPosition() const { return position; }
    float getAngle() const { return angle; }
    float getT() const { return t; }
};

class LineStateCoordinates {
public:
    static constexpr int NUM_IMAGE_SEEDS = 20;
    static constexpr float DEFAULT_WIDTH = 0.05f;

    enum class RenderType {
        LINE = 0,
        CONVEX = 1,
        CONCAVE = 2
    };

    struct RenderData {
        std::vector<float> texcoords;
        std::vector<float> positions;
    };

    LineStateCoordinates(const std::vector<AngledEdge>& edges, float scale = 1.0f, float length = 0.0f);

    // Core functionality
    std::vector<AngledEdge> getAngledEdges() const;
    float getScale() const;
    float getLength();
    float getAngle() const;

    // Operations
    LineStateCoordinates copy() const;
    Vec2* intersect(const LineStateCoordinates& other, float thickness1, float thickness2) const;
    RenderData getRenderData(Brush* brush = nullptr);

    // Static methods
    /*static RenderData getRenderDataOld(Brush* brush, const AngledEdge& seg0, const AngledEdge& seg3, 
                                     RenderType renderType = RenderType::LINE);
    static RenderData getRenderData(RenderData& renderData, Brush* brush, const AngledEdge& seg0, 
                                  const AngledEdge& seg3, const TileSeeds& tileSeeds, 
                                  RenderType renderType = RenderType::LINE);*/

private:
    std::vector<AngledEdge> angledEdges;
    float scale;
    float length;
    bool dirtyLength;
    bool dirtyRenderData;
    RenderData renderData;
    std::vector<int> imageSeeds;

    struct TileSeeds {
        int base;
        int increment;
    };

    // Helper methods
    void updateLength();
    void updateRenderData(Brush* brush);
    static int getTileImage(const std::vector<void*>& images, const TileSeeds& tileSeeds, int tileNum);
};

} // namespace ms 