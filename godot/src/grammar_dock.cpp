#include "grammar_editor.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/dir_access.hpp>

using namespace godot;

void GrammarDock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("on_load_grammar_pressed"), &GrammarDock::on_load_grammar_pressed);
	ClassDB::bind_method(D_METHOD("on_file_selected"), &GrammarDock::on_file_selected);
	ClassDB::bind_method(D_METHOD("on_generate_pressed"), &GrammarDock::on_generate_pressed);
	ClassDB::bind_method(D_METHOD("on_step_pressed"), &GrammarDock::on_step_pressed);
}

GrammarDock::GrammarDock() {
	// Initialize pointers
	load_grammar_button = nullptr;
	generate_button = nullptr;
	step_button_ref = nullptr;
	selected_file_label = nullptr;
	status_label = nullptr;
	file_dialog = nullptr;
	selected_file_path = "";
}

GrammarDock::~GrammarDock() {
	// Cleanup will be handled by Godot
}

void GrammarDock::_ready() {
	UtilityFunctions::print("PMUGG Dock initializing...");
	setup_ui();
	setup_file_dialog();
}

void GrammarDock::setup_ui() {
	// Create main layout
	VBoxContainer* vbox = memnew(VBoxContainer);
	vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL); // Make main container expand to fill width
	add_child(vbox);
	
	// Title
	Label* title = memnew(Label);
	title->set_text("Graph Grammar Generator");
	title->add_theme_font_size_override("font_size", 16);
	vbox->add_child(title);
	
	// File selection section
	VBoxContainer* file_section = memnew(VBoxContainer);
	file_section->set_h_size_flags(Control::SIZE_EXPAND_FILL); // Make section expand to fill width
	vbox->add_child(file_section);
	
	Label* file_label = memnew(Label);
	file_label->set_text("Grammar File:");
	file_section->add_child(file_label);
	
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
	
	// Generate button
	generate_button = memnew(Button);
	generate_button->set_text("Generate Mesh");
	generate_button->set_disabled(true);
	generate_button->connect("pressed", Callable(this, "on_generate_pressed"));
	generate_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gen_section->add_child(generate_button);
	
	// Step button
	Button* step_button = memnew(Button);
	step_button->set_text("Step");
	step_button->set_disabled(true);
	step_button->connect("pressed", Callable(this, "on_step_pressed"));
	step_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gen_section->add_child(step_button);
	
	// Store reference to step button
	step_button_ref = step_button;
	
	// Status label
	status_label = memnew(Label);
	status_label->set_text("Ready to load grammar file");
	gen_section->add_child(status_label);
}

void GrammarDock::setup_file_dialog() {
	file_dialog = memnew(FileDialog);
	add_child(file_dialog);

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

void GrammarDock::on_load_grammar_pressed() {
	UtilityFunctions::print("Opening file browser...");
	file_dialog->popup_centered(Vector2i(800, 600));
}

void GrammarDock::on_file_selected(String path) {
	UtilityFunctions::print("Selected grammar file: ", path);
	
	// Update UI
	selected_file_label->set_text(path.get_file());
	selected_file_label->add_theme_color_override("font_color", Color(1, 1, 1));
	selected_file_path = path;
	
	// Initialize PMUGG with the selected file
	// Get the editor plugin instance to call PMUGG functions
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (editor_interface) {
		// Try to find our plugin instance
		// For now, we'll call the DLL functions directly
		UtilityFunctions::print("Initializing PMUGG with file: ", path);
		
		// Call the DLL initialize function
		const char* path_cstr = path.utf8().get_data();
		char result[1024];
		initialize(path_cstr, result, sizeof(result), 0);
		
		UtilityFunctions::print("PMUGG initialization result: ", result);
		status_label->set_text("Grammar loaded - ready to generate");
	} else {
		UtilityFunctions::print("No editor interface available");
		status_label->set_text("Error: No editor interface");
	}
	
	// Enable generate button
	generate_button->set_disabled(false);
	step_button_ref->set_disabled(false);
}

void GrammarDock::on_generate_pressed() {
	UtilityFunctions::print("Generating mesh...");
	
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	// Call PMUGG functions to generate the mesh
	UtilityFunctions::print("Resetting PMUGG with random seed...");
	reset((int)UtilityFunctions::randi());
	
	UtilityFunctions::print("Iterating PMUGG 5 steps...");
	iterate(5);
	
	int face_count = getNumFaces();
	UtilityFunctions::print("Generated ", face_count, " faces");
	
	// Create the mesh in the scene
	create_godot_mesh();
	
	// Update status
	status_label->set_text("Generated mesh with " + String::num_int64(face_count) + " faces!");
}

void GrammarDock::create_godot_mesh() {
	UtilityFunctions::print("Creating Godot mesh...");
	
	// Get current scene
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (!editor_interface) {
		UtilityFunctions::print("No editor interface available");
		return;
	}
	
	Node* current_scene = editor_interface->get_edited_scene_root();
	if (!current_scene) {
		UtilityFunctions::print("No active scene - mesh created but not added to scene tree");
		return;
	}
	
	// Create ArrayMesh
	Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	
	// Create a simple test triangle
	PackedVector3Array vertices;
	PackedInt32Array indices;
	PackedVector3Array normals;
	
	vertices.push_back(Vector3(0, 1, 0));
	vertices.push_back(Vector3(-1, 0, 0));
	vertices.push_back(Vector3(1, 0, 0));
	
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	
	normals.push_back(Vector3(0, 0, 1));
	normals.push_back(Vector3(0, 0, 1));
	normals.push_back(Vector3(0, 0, 1));
	
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_NORMAL] = normals;
	
	// Create the mesh surface
	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	
	// Create MeshInstance3D node
	MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
	mesh_instance->set_mesh(array_mesh);
	mesh_instance->set_name("GeneratedMesh_" + String::num_int64(UtilityFunctions::randi()));
	
	// Add to current scene
	current_scene->add_child(mesh_instance);
	mesh_instance->set_owner(current_scene);
	
	UtilityFunctions::print("Added generated mesh to scene: ", mesh_instance->get_name());
	UtilityFunctions::print("Mesh generation complete!");
	
	status_label->set_text("Mesh generated successfully!");
}

void GrammarDock::update_mesh() {
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
	
	// Update status
	status_label->set_text("Updated - " + String::num_int64(mesh.numVertices) + " vertices, " + String::num_int64(mesh.numFaces) + " faces");
	
	UtilityFunctions::print("Mesh updated successfully!");
}

void GrammarDock::on_step_pressed() {
	UtilityFunctions::print("Stepping PMUGG (iterating once)...");
	
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	// Iterate PMUGG once
	iterate(1);
	
	// Update the mesh in the scene
	update_mesh();
} 