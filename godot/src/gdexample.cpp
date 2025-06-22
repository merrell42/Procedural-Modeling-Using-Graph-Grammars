#include "gdexample.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void GDExample::_bind_methods() {
	// Bind the DLL wrapper methods to Godot
	ClassDB::bind_method(D_METHOD("initialize_pmugg", "file_path", "seed"), &GDExample::initialize_pmugg);
	ClassDB::bind_method(D_METHOD("reset_pmugg", "seed"), &GDExample::reset_pmugg);
	ClassDB::bind_method(D_METHOD("iterate_pmugg", "steps"), &GDExample::iterate_pmugg);
	ClassDB::bind_method(D_METHOD("get_face_count"), &GDExample::get_face_count);
	ClassDB::bind_method(D_METHOD("set_pmugg_size", "x", "y", "z"), &GDExample::set_pmugg_size);
}

GDExample::GDExample() {
	// Initialize any variables here.
	time_passed = 0.0;
	pmugg_initialized = false;
}

GDExample::~GDExample() {
	// Add your cleanup here.
}

void GDExample::_process(double delta) {
	/* time_passed += delta;

	Vector2 new_position = Vector2(500.0 + (500.0 * sin(time_passed * 2.0)), 500.0 + (500.0 * cos(time_passed * 1.5)));

	set_position(new_position);
	UtilityFunctions::print("Time passed: ", time_passed);
	if (!pmugg_initialized) {
		initialize_pmugg("C:/PMUGG/grammar data/2D Basic Shapes/square filled.json", 0);
		pmugg_initialized = true;
	} else {
		iterate_pmugg(1);
	}
	int face_count = get_face_count();
	UtilityFunctions::print("Face count: ", face_count); */
}

// Implement DLL wrapper methods
void GDExample::initialize_pmugg(String file_path, int seed) {
	const char* path_cstr = file_path.utf8().get_data();
	char result[1024]; // Adjust size as needed
	PMUGGWrapper::initialize_grammar(path_cstr, result, sizeof(result), seed);
}

void GDExample::reset_pmugg(int seed) {
	PMUGGWrapper::reset_grammar(seed);
}

void GDExample::iterate_pmugg(int steps) {
	PMUGGWrapper::iterate_grammar(steps);
}

int GDExample::get_face_count() {
	return PMUGGWrapper::get_num_faces();
}

void GDExample::set_pmugg_size(float x, float y, float z) {
	PMUGGWrapper::set_size(x, y, z);
}