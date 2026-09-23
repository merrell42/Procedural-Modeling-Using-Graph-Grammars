#pragma once
#include <map>
#include <string>
#include <vector>
#include "../geometry/instance.h"
#include "../geometry/matrix4.h"

using namespace std;

class VertexDecoration {
    public:
        virtual ~VertexDecoration() = default;
        const string& getId() const { return id; }
        void setId(const string& id) { this->id = id; }
        virtual vector<Instance> getInstances(const Matrix4& transform) const = 0;
        virtual vector<VertexDecoration*> getChildren() const { return {}; }
        virtual void resolveChildren(const map<string, VertexDecoration*>&) {}

    protected:
        string id;
};
