#pragma once
#include <vector>
#include "vertex_decoration.h"
#include "edge_decoration.h"
#include "face_decoration.h"
#include "../util/util.h"

using namespace std;

template <typename Base, typename... Args>
class PickRandomDecoration : public Base {
public:
    void addChild(Base* child, double weight = 1.0) {
        if (child && child != this) {
            children.push_back(child);
            weights.push_back(weight);
        }
    }

    void setUseWeights(bool useWeights) {
        this->useWeights = useWeights;
    }

    vector<Instance> getInstances(Args... args) const override {
        if (children.empty()) {
            return {};
        }

        int index = -1;
        if (useWeights) {
            index = Util::randomDistribution(weights);
        } else {
            index = Util::randomInt((int)children.size());
        }
        if (index < 0 || index >= (int)children.size() || !children[index]) {
            return {};
        }
        return children[index]->getInstances(args...);
    }

private:
    vector<Base*> children;
    vector<double> weights;
    bool useWeights = false;
};

using VertexPickRandomDecoration = PickRandomDecoration<VertexDecoration, const Matrix4&>;
using EdgePickRandomDecoration = PickRandomDecoration<EdgeDecoration, const Vec3&, const Vec3&>;
using FacePickRandomDecoration = PickRandomDecoration<FaceDecoration, const Face&>;
