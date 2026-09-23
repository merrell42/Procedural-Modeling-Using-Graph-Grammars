#pragma once
#include <vector>
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"

using namespace std;

template <typename Base, typename... Args>
class UnionDecoration : public Base {
public:
    void addChild(Base* child) {
        if (child && child != this) {
            children.push_back(child);
        }
    }

    vector<Instance> getInstances(Args... args) const override {
        vector<Instance> collected;
        for (Base* child : children) {
            if (!child) {
                continue;
            }
            vector<Instance> instances = child->getInstances(args...);
            collected.insert(collected.end(), instances.begin(), instances.end());
        }
        return collected;
    }

private:
    vector<Base*> children;
};

using VertexUnionDecoration = UnionDecoration<VertexDecoration, const Matrix4&>;
using EdgeUnionDecoration = UnionDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
using FaceUnionDecoration = UnionDecoration<FaceDecoration, const Face&>;
