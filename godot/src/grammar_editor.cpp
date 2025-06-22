#include "grammar_editor.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

using namespace godot;

void GrammarEditor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize_pmugg_editor", "file_path", "seed"), &GrammarEditor::initialize_pmugg_editor);
    ClassDB::bind_method(D_METHOD("reset_pmugg_editor", "seed"), &GrammarEditor::reset_pmugg_editor);
    ClassDB::bind_method(D_METHOD("iterate_pmugg_editor", "steps"), &GrammarEditor::iterate_pmugg_editor);
    ClassDB::bind_method(D_METHOD("get_face_count_editor"), &GrammarEditor::get_face_count_editor);
    ClassDB::bind_method(D_METHOD("set_pmugg_size_editor", "x", "y", "z"), &GrammarEditor::set_pmugg_size_editor);
}

GrammarEditor::GrammarEditor() {
    pmugg_initialized = false;
    mesh_instance = nullptr;
    dock = nullptr;
}

GrammarEditor::~GrammarEditor() {}

void GrammarEditor::_enter_tree() {
    // Create and add the dock to the left upper dock slot
    dock = memnew(GrammarDock);
    dock->set_h_size_flags(Control::SIZE_EXPAND_FILL); // Make dock expand to fill available width
    dock->set_custom_minimum_size(Vector2(200, 0)); // Set minimum width
    add_control_to_dock(DOCK_SLOT_LEFT_UL, dock);
    
    // Force layout update
    dock->force_update_transform();

    // Initialize PMUGG with a default grammar
    initialize_pmugg_editor("C:/PMUGG/grammar data/2D Basic Shapes/square filled.json", 0);
}

void GrammarEditor::_exit_tree() {
    if (dock) {
        remove_control_from_bottom_panel(dock);
        dock->queue_free();
        dock = nullptr;
    }
    
    // Clean up mesh instance if it exists
    if (mesh_instance && mesh_instance->is_inside_tree()) {
        mesh_instance->queue_free();
    }
}

void GrammarEditor::initialize_pmugg_editor(String file_path, int seed) {
    const char* path_cstr = file_path.utf8().get_data();
    char result[1024];
    initialize(path_cstr, result, sizeof(result), seed);
    pmugg_initialized = true;
    UtilityFunctions::print("PMUGG initialized with: ", file_path);
}

void GrammarEditor::reset_pmugg_editor(int seed) {
    if (pmugg_initialized) {
        reset(seed);
        UtilityFunctions::print("PMUGG reset with seed: ", seed);
    }
}

void GrammarEditor::iterate_pmugg_editor(int steps) {
    if (pmugg_initialized) {
        iterate(steps);
        UtilityFunctions::print("PMUGG iterated ", steps, " steps. Face count: ", get_face_count_editor());
    }
}

int GrammarEditor::get_face_count_editor() {
    if (pmugg_initialized) {
        return getNumFaces();
    }
    return 0;
}

void GrammarEditor::set_pmugg_size_editor(float x, float y, float z) {
    if (pmugg_initialized) {
        setSize(x, y, z);
        UtilityFunctions::print("PMUGG size set to: (", x, ", ", y, ", ", z, ")");
    }
}
