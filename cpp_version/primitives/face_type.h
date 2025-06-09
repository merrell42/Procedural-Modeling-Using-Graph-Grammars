#pragma once
#include <vector>
#include <string>
#include "../geometry/vec3.h"

class View;

class FaceType {
public:
    FaceType(const string& material, const Vec3& normal);
    ~FaceType() = default;

    const string& getMaterial() const;
    const Vec3& getNormal() const;
    const Vec3& getU() const;
    const Vec3& getV() const;
    bool isMonotonic() const;
    const Vec3* getColor() const;
    int getId() const;
    int getMaxDim() const;

    void setColor(Vec3* newColor);
    void setMonotonic(bool isMonotonic);

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

