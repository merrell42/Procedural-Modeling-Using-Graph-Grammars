#pragma once
#include <map>
#include <string>
#include <vector>
#include "../geometry/instance.h"
#include "../geometry/vec3.h"

using namespace std;

class EdgeDecoration {
    public:
        virtual ~EdgeDecoration() = default;
        const string& getId() const { return id; }
        void setId(const string& id) { this->id = id; }
        virtual vector<Instance> getInstances(const Vec3& start, const Vec3& end) const = 0;
        virtual vector<EdgeDecoration*> getChildren() const { return {}; }
        virtual void resolveChildren(const map<string, EdgeDecoration*>&) {}

    protected:
        string id;
};
