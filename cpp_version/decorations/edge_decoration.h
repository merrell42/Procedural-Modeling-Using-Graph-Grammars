#pragma once
#include <string>
#include <vector>
#include "decoration_output.h"
#include "../geometry/vec3.h"

using namespace std;

class Decorations;

class EdgeDecoration {
    public:
        virtual ~EdgeDecoration() = default;
        const string& getId() const { return id; }
        void setId(const string& id) { this->id = id; }
        virtual DecorationOutput getOutput(const Vec3& start, const Vec3& end) const = 0;
        virtual vector<EdgeDecoration*> getChildren() const { return {}; }
        virtual void resolveChildren(const Decorations&) {}

    protected:
        string id;
};
