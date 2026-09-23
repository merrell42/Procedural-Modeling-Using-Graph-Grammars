#pragma once
#include <map>
#include <string>
#include <vector>
#include "decoration_template.h"
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"

using namespace std;

DECORATION_TEMPLATE
class UnionDecoration : public Base {
    public:
        void addChild(Base* child);
        vector<Base*> getChildren() const override;
        void resolveChildren(const map<string, Base*>& decorations) override;
        vector<Instance> getInstances(Args... args) const override;

    private:
        vector<Base*> children;
};

DECORATION_TEMPLATE
using Union = UnionDecoration<Base, Args...>;

using VertexUnionDecoration = Union<VertexDecoration, const Matrix4&>;
using EdgeUnionDecoration = Union<EdgeDecoration, const Vec3&, const Vec3&>;
using FaceUnionDecoration = Union<FaceDecoration, const Face&>;
