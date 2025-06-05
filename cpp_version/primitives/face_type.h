#pragma once
#include <vector>
#include <string>
#include "../geometry/vec3.h"

namespace ms {

class View;

class FaceType {
public:
    FaceType(const string& material, const Vec3& normal);
    ~FaceType() = default;

    const string& getMaterial() const { return material; }
    const Vec3& getNormal() const { return normal; }
    const Vec3& getU() const { return u; }
    const Vec3& getV() const { return v; }
    bool isMonotonic() const { return monotonic; }
    const Vec3* getColor() const { return color; }
    int getId() const { return id; }
    int getMaxDim() const { return maxDim; }

    void setColor(Vec3* newColor) { color = newColor; }
    void setMonotonic(bool isMonotonic) { monotonic = isMonotonic; }

    double angle(const Vec3& q) const;
    double getArea(const vector<Vec3>& vertices) const;
    Vec3 normalColor() const;

    static FaceType* import(const Json& json);

    static constexpr double EPS = 1e-5;

    Vec3 u;
    Vec3 v;
    int id;

private:
    string material;
    Vec3 normal;
    bool monotonic;
    Vec3* color;
    int maxDim;

    static int nextId;

    // Helper methods
    void computeOrthonormalBasis();
    double polygonArea(const vector<Vec2>& points) const;
};

} // namespace ms 