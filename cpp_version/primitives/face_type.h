#pragma once
#include <optional>
#include <vector>
#include <string>
#include <iosfwd>
#include "../geometry/vec3.h"

class View;
class Primitives;
class FaceDecoration;

class FaceType {
public:
    FaceType(const string& material, const Vec3& normal);
    ~FaceType() = default;

    const string& getMaterial() const;
    FaceDecoration* getDecoration() const;
    void setDecoration(FaceDecoration* decoration);
    const Vec3& getNormal() const;
    const Vec3& getU() const;
    const Vec3& getV() const;
    const Vec3& getColor() const;
    double getGridSpacing() const;
    double getNoiseScale() const;
    double getNoiseIntensity() const;
    int getMaxDim() const;
    double angle(const Vec3& dir) const;

    static FaceType* import(const Json& json, Primitives* shape);
    static FaceType* binaryDeserialize(std::istream& in);
    Json exportJson(const Primitives* shape) const;

private:
    string material;
    FaceDecoration* decoration = nullptr;
    Vec3 normal;
    Vec3 u;
    Vec3 v;
    Vec3 color;
    int maxDim;

    // Set only when this face type names its own value. Otherwise uses globalSettings.
    std::optional<double> gridSpacing;
    std::optional<double> noiseScale;
    std::optional<double> noiseIntensity;
};
