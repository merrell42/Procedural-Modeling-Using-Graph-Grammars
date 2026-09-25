#include "pch.h"
#include "place_object_decoration.h"

PlaceObjectDecoration::PlaceObjectDecoration(const string& object)
    : object(object) {
}

DecorationOutput PlaceObjectDecoration::getOutput(const Matrix4& transform) const {
    DecorationOutput output;
    Instance instance;
    instance.assetId = object.c_str();
    instance.transform = transform;
    output.instances.push_back(instance);
    return output;
}
