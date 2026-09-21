#include "pch.h"
#include "place_object_decoration.h"

PlaceObjectDecoration::PlaceObjectDecoration(const string& object)
    : object(object) {
}

vector<Instance> PlaceObjectDecoration::getInstances(const Matrix4& transform) const {
    Instance instance;
    instance.assetId = object.c_str();
    instance.transform = transform;
    return {instance};
}
