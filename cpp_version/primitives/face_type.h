#pragma once
#include <vector>
#include <string>
#include <iosfwd>
#include "../geometry/vec3.h"

class View;

class FaceType {
public:
    FaceType(const string& material, const Vec3& normal);
    ~FaceType() = default;

    const string& getMaterial() const;
    const Vec3& getNormal() const;
    const Vec3& getColor() const;
    int getMaxDim() const;
    double angle(const Vec3& dir) const;

    static FaceType* import(const Json& json);
    static FaceType* binaryDeserialize(std::istream& in);
    Json exportJson() const;

private:
    static void orthonormalBasis(const Vec3& normal, Vec3& u, Vec3& v);

    string material;
    Vec3 normal;
    Vec3 u;
    Vec3 v;
    Vec3 color;
    int maxDim;
};
