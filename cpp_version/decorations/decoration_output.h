#pragma once
#include <vector>
#include "../geometry/instance.h"
#include "../geometry/vec3.h"
#include "../util/util.h"

using namespace std;

struct Color {
    float red;
    float green;
    float blue;

    bool operator<(const Color& other) const {
        if (red != other.red) {
            return red < other.red;
        }
        if (green != other.green) {
            return green < other.green;
        }
        return blue < other.blue;
    }
};

struct Extrusion {
    vector<Vec3> positions;
    vector<Vec3> normals;
    vector<int> triangles;
    Color color = { 1, 1, 1 };
};

struct DecorationOutput {
    vector<Instance> instances;
    vector<Extrusion> extrusions;

    void append(const DecorationOutput& other) {
        Util::append(instances, other.instances);
        Util::append(extrusions, other.extrusions);
    }
};
