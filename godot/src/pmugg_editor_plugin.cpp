#include "pmugg_editor_plugin.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

using namespace godot;

void PMUGGEditorPlugin::_bind_methods() {
    // Bind methods to be accessible from editor scripts
    ClassDB::bind_method(D_METHOD("initialize_pmugg_editor", "file_path", "seed"), &PMUGGEditorPlugin::initialize_pmugg_editor);
    ClassDB::bind_method(D_METHOD("reset_pmugg_editor", "seed"), &PMUGGEditorPlugin::reset_pmugg_editor);
    ClassDB::bind_method(D_METHOD("iterate_pmugg_editor", "steps"), &PMUGGEditorPlugin::iterate_pmugg_editor);
    ClassDB::bind_method(D_METHOD("generate_mesh_in_editor"), &PMUGGEditorPlugin::generate_mesh_in_editor);
    ClassDB::bind_method(D_METHOD("get_face_count_editor"), &PMUGGEditorPlugin::get_face_count_editor);
    ClassDB::bind_method(D_METHOD("set_pmugg_size_editor", "x", "y", "z"), &PMUGGEditorPlugin::set_pmugg_size_editor);
}

PMUGGEditorPlugin::PMUGGEditorPlugin() {
    pmugg_initialized = false;
    mesh_instance = nullptr;
    dock = nullptr;
}

PMUGGEditorPlugin::~PMUGGEditorPlugin() {
    // Cleanup if needed
}

void PMUGGEditorPlugin::_enter_tree() {
    UtilityFunctions::print("PMUGG Editor Plugin activated!");
    
    // Create and add the dock to the left upper dock slot
    dock = memnew(PMUGGDock);
    dock->set_h_size_flags(Control::SIZE_EXPAND_FILL); // Make dock expand to fill available width
    dock->set_custom_minimum_size(Vector2(200, 0)); // Set minimum width
    add_control_to_dock(DOCK_SLOT_LEFT_UL, dock);
    
    // Force layout update
    dock->force_update_transform();
    
    // Initialize PMUGG with a default grammar
    initialize_pmugg_editor("C:/PMUGG/grammar data/2D Basic Shapes/square filled.json", 0);
    
    // Create initial mesh
    generate_mesh_in_editor();
}

void PMUGGEditorPlugin::_exit_tree() {
    UtilityFunctions::print("PMUGG Editor Plugin deactivated!");
    
    // Remove the dock
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

void PMUGGEditorPlugin::initialize_pmugg_editor(String file_path, int seed) {
    const char* path_cstr = file_path.utf8().get_data();
    char result[1024];
    PMUGGWrapper::initialize_grammar(path_cstr, result, sizeof(result), seed);
    pmugg_initialized = true;
    UtilityFunctions::print("PMUGG initialized with: ", file_path);
}

void PMUGGEditorPlugin::reset_pmugg_editor(int seed) {
    if (pmugg_initialized) {
        PMUGGWrapper::reset_grammar(seed);
        generate_mesh_in_editor(); // Automatically regenerate mesh
        UtilityFunctions::print("PMUGG reset with seed: ", seed);
    }
}

void PMUGGEditorPlugin::iterate_pmugg_editor(int steps) {
    if (pmugg_initialized) {
        PMUGGWrapper::iterate_grammar(steps);
        generate_mesh_in_editor(); // Automatically regenerate mesh
        UtilityFunctions::print("PMUGG iterated ", steps, " steps. Face count: ", get_face_count_editor());
    }
}

int PMUGGEditorPlugin::get_face_count_editor() {
    if (pmugg_initialized) {
        return PMUGGWrapper::get_num_faces();
    }
    return 0;
}

void PMUGGEditorPlugin::set_pmugg_size_editor(float x, float y, float z) {
    if (pmugg_initialized) {
        PMUGGWrapper::set_size(x, y, z);
        generate_mesh_in_editor(); // Automatically regenerate mesh
        UtilityFunctions::print("PMUGG size set to: (", x, ", ", y, ", ", z, ")");
    }
}

void PMUGGEditorPlugin::generate_mesh_in_editor() {
    if (!pmugg_initialized) {
        return;
    }

    // Get current scene
    Node *current_scene = get_editor_interface()->get_edited_scene_root();
    if (!current_scene) {
        UtilityFunctions::print("No scene open in editor");
        return;
    }

    // Create or find mesh instance
    if (!mesh_instance) {
        mesh_instance = memnew(MeshInstance3D);
        mesh_instance->set_name("PMUGG Generated Mesh");
        current_scene->add_child(mesh_instance);
        
        // Set owner to make it part of the scene
        mesh_instance->set_owner(current_scene);
    }

    // Get the mesh from PMUGG
    MeshCpp mesh = PMUGGWrapper::get_mesh();
    
    // Print mesh information
    UtilityFunctions::print("PMUGG mesh - Vertices: ", mesh.numVertices, ", Faces: ", mesh.numFaces);
    
    // For now, create a simple test mesh since we're just testing the data
    Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    
    // Create a simple triangle for testing
    PackedVector3Array vertices;
    PackedInt32Array indices;
    
    vertices.push_back(Vector3(0, 1, 0));
    vertices.push_back(Vector3(-1, 0, 0));
    vertices.push_back(Vector3(1, 0, 0));
    
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_INDEX] = indices;
    
    array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    mesh_instance->set_mesh(array_mesh);
    
    // Create a material
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(Color(0.8, 0.3, 0.3)); // Red color
    mesh_instance->set_material_override(material);
    
    UtilityFunctions::print("Generated test mesh while PMUGG reports ", mesh.numVertices, " vertices and ", mesh.numFaces, " faces");
} 