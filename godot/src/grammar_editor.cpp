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
    // Bind methods to be accessible from editor scripts
    ClassDB::bind_method(D_METHOD("initialize_pmugg_editor", "file_path", "seed"), &GrammarEditor::initialize_pmugg_editor);
    ClassDB::bind_method(D_METHOD("reset_pmugg_editor", "seed"), &GrammarEditor::reset_pmugg_editor);
    ClassDB::bind_method(D_METHOD("iterate_pmugg_editor", "steps"), &GrammarEditor::iterate_pmugg_editor);
    ClassDB::bind_method(D_METHOD("get_face_count_editor"), &GrammarEditor::get_face_count_editor);
    ClassDB::bind_method(D_METHOD("set_pmugg_size_editor", "x", "y", "z"), &GrammarEditor::set_pmugg_size_editor);
    
    // Bind UI event handlers
    ClassDB::bind_method(D_METHOD("on_load_grammar_pressed"), &GrammarEditor::on_load_grammar_pressed);
    ClassDB::bind_method(D_METHOD("on_file_selected"), &GrammarEditor::on_file_selected);
    ClassDB::bind_method(D_METHOD("on_step_pressed"), &GrammarEditor::on_step_pressed);
}

GrammarEditor::GrammarEditor() {
    pmugg_initialized = false;
    mesh_instance = nullptr;
    
    // Initialize UI pointers
    load_grammar_button = nullptr;
    step_button_ref = nullptr;
    selected_file_label = nullptr;
    file_dialog = nullptr;
    selected_file_path = "";
}

GrammarEditor::~GrammarEditor() {
    // Cleanup if needed
}

void GrammarEditor::_enter_tree() {
    UtilityFunctions::print("Grammar Editor Plugin activated!");
    
    // Create a container for the UI
    Control* ui_container = memnew(Control);
    ui_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    ui_container->set_custom_minimum_size(Vector2(200, 0));
    
    // Create the UI in the container
    setup_ui(ui_container);
    setup_file_dialog(ui_container);
    
    // Add the UI container to a dock slot
    add_control_to_dock(DOCK_SLOT_LEFT_UL, ui_container);
    
    // Initialize PMUGG with a default grammar
    initialize_pmugg_editor("C:/PMUGG/grammar data/2D Basic Shapes/square filled.json", 0);
}

void GrammarEditor::_exit_tree() {
    UtilityFunctions::print("Grammar Editor Plugin deactivated!");
    
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

void GrammarEditor::setup_ui(Control* parent) {
	// Create main layout
	VBoxContainer* vbox = memnew(VBoxContainer);
	vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	parent->add_child(vbox);
	
	// Title
	Label* title = memnew(Label);
	title->set_text("Graph Grammar Editor");
	title->add_theme_font_size_override("font_size", 16);
	vbox->add_child(title);
	
	// File selection section
	VBoxContainer* file_section = memnew(VBoxContainer);
	file_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(file_section);
	
	// Selected file display
	selected_file_label = memnew(Label);
	selected_file_label->set_text("No file selected");
	selected_file_label->add_theme_color_override("font_color", Color(0.5, 0.5, 0.5));
	file_section->add_child(selected_file_label);
	
	// Load Grammar button
	load_grammar_button = memnew(Button);
	load_grammar_button->set_text("Load Grammar...");
	load_grammar_button->connect("pressed", Callable(this, "on_load_grammar_pressed"));
	load_grammar_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	file_section->add_child(load_grammar_button);
	
	// Generation section
	VBoxContainer* gen_section = memnew(VBoxContainer);
	gen_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(gen_section);
	
	// Step button
	Button* step_button = memnew(Button);
	step_button->set_text("Step");
	step_button->set_disabled(true);
	step_button->connect("pressed", Callable(this, "on_step_pressed"));
	step_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gen_section->add_child(step_button);
	step_button_ref = step_button;
}

void GrammarEditor::setup_file_dialog(Control* parent) {
	file_dialog = memnew(FileDialog);
	parent->add_child(file_dialog);

	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);

	file_dialog->add_filter("*.json", "Grammar Files");
	String base_dir = ProjectSettings::get_singleton()->globalize_path("res://").get_base_dir();
	String grammar_dir = base_dir + "/../../grammar data";
	
	if (DirAccess::dir_exists_absolute(grammar_dir)) {
		file_dialog->set_current_dir(grammar_dir);
	} else {
		UtilityFunctions::print("Grammar data directory is not found at: ", grammar_dir);
		file_dialog->set_current_dir(base_dir);
	}
	file_dialog->connect("file_selected", Callable(this, "on_file_selected"));
}

void GrammarEditor::on_load_grammar_pressed() {
	UtilityFunctions::print("Opening file browser...");
	file_dialog->popup_centered(Vector2i(800, 600));
}

void GrammarEditor::on_file_selected(String path) {
	UtilityFunctions::print("Selected grammar file: ", path);
	
	// Update UI
	selected_file_label->set_text(path.get_file());
	selected_file_label->add_theme_color_override("font_color", Color(1, 1, 1));
	selected_file_path = path;
	
	// Initialize PMUGG with the selected file
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (editor_interface) {
		UtilityFunctions::print("Initializing PMUGG with file: ", path);
		
		// Call the DLL initialize function
		const char* path_cstr = path.utf8().get_data();
		char result[1024];
		initialize(path_cstr, result, sizeof(result), 0);
		
		UtilityFunctions::print("PMUGG initialization result: ", result);
	} else {
		UtilityFunctions::print("No editor interface available");
	}
	
	// Enable buttons
	step_button_ref->set_disabled(false);
}

void GrammarEditor::on_step_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	iterate(1);
	update_mesh();
}

void GrammarEditor::update_mesh() {
	UtilityFunctions::print("Updating mesh...");
	
	// Get current mesh info
	MeshCpp mesh = getMesh();
	UtilityFunctions::print("Vertices: ", mesh.numVertices, ", Faces: ", mesh.numFaces);
	
	// Get current scene
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (!editor_interface) {
		UtilityFunctions::print("No editor interface available");
		return;
	}
	
	Node* current_scene = editor_interface->get_edited_scene_root();
	if (!current_scene) {
		UtilityFunctions::print("No active scene - mesh updated but not added to scene tree");
		return;
	}
	
	// Find existing mesh instance or create new one
	MeshInstance3D* mesh_instance = nullptr;
	for (int i = 0; i < current_scene->get_child_count(); i++) {
		Node* child = current_scene->get_child(i);
		if (child->get_class() == "MeshInstance3D" && child->get_name().begins_with("GeneratedMesh_")) {
			mesh_instance = Object::cast_to<MeshInstance3D>(child);
			break;
		}
	}
	
	if (!mesh_instance) {
		// Create new mesh instance if none exists
		mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_name("GeneratedMesh_" + String::num_int64(UtilityFunctions::randi()));
		current_scene->add_child(mesh_instance);
		mesh_instance->set_owner(current_scene);
	}
	
	// Create ArrayMesh with current PMUGG data
	Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	
	// Use real PMUGG mesh data
	PackedVector3Array vertices;
	PackedInt32Array indices;
	PackedVector3Array normals;
	
	// Convert PMUGG positions to Godot vertices (switch Y and Z like Unity does)
	vertices.resize(mesh.numVertices);
	for (int i = 0; i < mesh.numVertices; i++) {
		// Switch Y and Z coordinates like Unity does
		vertices[i] = Vector3(mesh.positions[i * 3], mesh.positions[i * 3 + 2], mesh.positions[i * 3 + 1]);
	}
	
	// Convert PMUGG normals to Godot normals (switch Y and Z like Unity does)
	normals.resize(mesh.numVertices);
	for (int i = 0; i < mesh.numVertices; i++) {
		// Switch Y and Z coordinates like Unity does
		normals[i] = Vector3(mesh.normals[i * 3], mesh.normals[i * 3 + 2], mesh.normals[i * 3 + 1]);
	}
	
	// Convert PMUGG triangles to Godot indices
	indices.resize(mesh.numTriangles * 3);
	for (int i = 0; i < mesh.numTriangles; i++) {
		// Reverse winding order for each triangle (swap indices 1 and 2)
		indices[i * 3] = mesh.triangles[i * 3];     // Keep first vertex
		indices[i * 3 + 1] = mesh.triangles[i * 3 + 2]; // Swap second and third
		indices[i * 3 + 2] = mesh.triangles[i * 3 + 1]; // Swap second and third
	}
	
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	
	// Create the mesh surface
	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	mesh_instance->set_mesh(array_mesh);
	
	UtilityFunctions::print("Mesh updated successfully!");
}
