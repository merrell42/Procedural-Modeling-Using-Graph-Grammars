#pragma once
#include <string>
#include <vector>
#include "decoration_output.h"
#include "../geometry/matrix4.h"

using namespace std;

class Decorations;

class VertexDecoration {
    public:
        virtual ~VertexDecoration() = default;
        const string& getId() const { return id; }
        void setId(const string& id) { this->id = id; }
        virtual DecorationOutput getOutput(const Matrix4& transform) const = 0;
        virtual vector<VertexDecoration*> getChildren() const { return {}; }
        virtual void resolveChildren(const Decorations&) {}

    protected:
        string id;
};
