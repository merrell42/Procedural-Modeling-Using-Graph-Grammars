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
    int getMaxDim() const;

    static FaceType* import(const Json& json);

    static constexpr double EPS = 1e-5;

    Vec3 u;
    Vec3 v;
    int id;

private:
    string material;
    Vec3 normal;
    Vec3* color;
    int maxDim;

    static int nextId;

    // Helper methods
    void computeOrthonormalBasis();
};

