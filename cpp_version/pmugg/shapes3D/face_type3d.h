#pragma once
#include <vector>
#include <string>
#include "../shape/vec3.h"

namespace ms {

class View;

class FaceType3D {
public:
    FaceType3D(const std::string& material, const Vec3& normal);
    ~FaceType3D() = default;

    // Core accessors
    const std::string& getMaterial() const { return material; }
    const Vec3& getNormal() const { return normal; }
    const Vec3& getU() const { return u; }
    const Vec3& getV() const { return v; }
    bool isMonotonic() const { return monotonic; }
    const Vec3* getColor() const { return color; }
    int getId() const { return id; }
    int getMaxDim() const { return maxDim; }

    // Modifiers
    void setColor(Vec3* newColor) { color = newColor; }
    void setMonotonic(bool isMonotonic) { monotonic = isMonotonic; }

    // Operations
    float angle(const Vec3& q) const;
    float getArea(const std::vector<Vec3>& vertices) const;
    // void computeMonotonic(const std::vector<Graph*>& graphs);
    Vec3 normalColor() const;

    // Import/Export
    // Json export() const;
    static FaceType3D* import(const Json& json);
    static FaceType3D* partialImport(const Json& json);

    static constexpr float EPS = 1e-5f;

    Vec3 u;
    Vec3 v;
    int id;

private:
    std::string material;
    Vec3 normal;
    bool monotonic;
    Vec3* color;
    int maxDim;

    static int nextId;

    // Helper methods
    void computeOrthonormalBasis();
    float polygonArea(const std::vector<Vec2>& points) const;
};

} // namespace ms 