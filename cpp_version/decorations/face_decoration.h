#pragma once
#include <string>
#include <vector>
#include "../geometry/instance.h"

using namespace std;

class Decorations;
class Face;

class FaceDecoration {
    public:
        virtual ~FaceDecoration() = default;
        const string& getId() const { return id; }
        void setId(const string& id) { this->id = id; }
        virtual vector<Instance> getInstances(const Face& face) const = 0;
        virtual vector<FaceDecoration*> getChildren() const { return {}; }
        virtual void resolveChildren(const Decorations&) {}

    protected:
        string id;
};
