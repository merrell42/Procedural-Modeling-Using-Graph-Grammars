#include "pmugg_wrapper.h"

using namespace godot;

void PMUGGWrapper::initialize_grammar(const char* filePath, char* result, int len, int seed) {
    initialize(filePath, result, len, seed);
}

void PMUGGWrapper::reset_grammar(int seed) {
    reset(seed);
}

void PMUGGWrapper::iterate_grammar(int steps) {
    iterate(steps);
}

int PMUGGWrapper::get_num_faces() {
    return getNumFaces();
}

MeshCpp PMUGGWrapper::get_mesh() {
    return getMesh();
}

void PMUGGWrapper::set_size(float x, float y, float z) {
    setSize(x, y, z);
}

void PMUGGWrapper::destroy_mesh(MeshCpp& mesh) {
    destroyMesh(mesh);
} 