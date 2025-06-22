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
    ClassDB::bind_method(D_METHOD("on_load_grammar_pressed"), &GrammarEditor::on_load_grammar_pressed);
    ClassDB::bind_method(D_METHOD("on_file_selected"), &GrammarEditor::on_file_selected);
    ClassDB::bind_method(D_METHOD("on_step_pressed"), &GrammarEditor::on_step_pressed);
    ClassDB::bind_method(D_METHOD("on_reset_pressed"), &GrammarEditor::on_reset_pressed);
    ClassDB::bind_method(D_METHOD("on_play_pressed"), &GrammarEditor::on_play_pressed);
    ClassDB::bind_method(D_METHOD("on_animation_timer_timeout"), &GrammarEditor::on_animation_timer_timeout);
    ClassDB::bind_method(D_METHOD("on_size_x_changed"), &GrammarEditor::on_size_x_changed);
    ClassDB::bind_method(D_METHOD("on_size_y_changed"), &GrammarEditor::on_size_y_changed);
    ClassDB::bind_method(D_METHOD("on_size_z_changed"), &GrammarEditor::on_size_z_changed);
}

GrammarEditor::GrammarEditor() {
    mesh_instance = nullptr;

    load_grammar_button = nullptr;
    step_button = nullptr;
    selected_file_label = nullptr;
    file_dialog = nullptr;
    selected_file_path = "";
    
    is_playing = false;
    animation_timer = nullptr;
    
    // Initialize values
    seed_value = 0;
    size_x_value = 30.0f;
    size_y_value = 20.0f;
    size_z_value = 10.0f;
}

GrammarEditor::~GrammarEditor() {}

void GrammarEditor::_enter_tree() {
    // Create a container for the UI
    Control* ui_container = memnew(Control);
    ui_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    ui_container->set_custom_minimum_size(Vector2(200, 0));
    
    // Create the UI in the container
    setup_ui(ui_container);
    setup_file_dialog(ui_container);
    
    // Add the UI container to a dock slot
    add_control_to_dock(DOCK_SLOT_LEFT_UL, ui_container);
}

void GrammarEditor::_exit_tree() {
    // Clean up mesh instance if it exists
    if (mesh_instance && mesh_instance->is_inside_tree()) {
        mesh_instance->queue_free();
    }
}

/* void GrammarEditor::initialize_pmugg_editor(String file_path, int seed) {
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
} */

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
	load_grammar_button->add_theme_font_size_override("font_size", 14);
	file_section->add_child(load_grammar_button);
	
	// Generation section
	VBoxContainer* gen_section = memnew(VBoxContainer);
	gen_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(gen_section);
	
	// Play button
	play_button = memnew(Button);
	play_button->set_text("Play");
	play_button->set_disabled(true);
	play_button->connect("pressed", Callable(this, "on_play_pressed"));
	play_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	play_button->add_theme_font_size_override("font_size", 14);
	gen_section->add_child(play_button);
	
	// Reset button
	reset_button = memnew(Button);
	reset_button->set_text("Reset");
	reset_button->set_disabled(true);
	reset_button->connect("pressed", Callable(this, "on_reset_pressed"));
	reset_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	reset_button->add_theme_font_size_override("font_size", 14);
	gen_section->add_child(reset_button);
	
	// Step button
	step_button = memnew(Button);
	step_button->set_text("Step");
	step_button->set_disabled(true);
	step_button->connect("pressed", Callable(this, "on_step_pressed"));
	step_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	step_button->add_theme_font_size_override("font_size", 14);
	gen_section->add_child(step_button);
	
	// Parameters section
	VBoxContainer* params_section = memnew(VBoxContainer);
	params_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(params_section);
	
	// Seed input
	HBoxContainer* seed_container = memnew(HBoxContainer);
	seed_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_section->add_child(seed_container);
	
	Label* seed_label = memnew(Label);
	seed_label->set_text("Seed:");
	seed_label->set_custom_minimum_size(Vector2(50, 0));
	seed_container->add_child(seed_label);
	
	seed_input = memnew(LineEdit);
	seed_input->set_text("0");
	seed_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	seed_input->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	seed_input->add_theme_font_size_override("font_size", 12);
	seed_container->add_child(seed_input);
	
	// Size inputs
	HBoxContainer* size_container = memnew(HBoxContainer);
	size_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	params_section->add_child(size_container);
	
	Label* size_label = memnew(Label);
	size_label->set_text("Size:");
	size_label->set_custom_minimum_size(Vector2(50, 0));
	size_container->add_child(size_label);
	
	// Size X
	size_x_input = memnew(LineEdit);
	size_x_input->set_text("30.0");
	size_x_input->set_custom_minimum_size(Vector2(60, 0));
	size_x_input->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	size_x_input->add_theme_font_size_override("font_size", 12);
	size_x_input->connect("text_changed", Callable(this, "on_size_x_changed"));
	size_container->add_child(size_x_input);
	
	// Size Y
	size_y_input = memnew(LineEdit);
	size_y_input->set_text("20.0");
	size_y_input->set_custom_minimum_size(Vector2(60, 0));
	size_y_input->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	size_y_input->add_theme_font_size_override("font_size", 12);
	size_y_input->connect("text_changed", Callable(this, "on_size_y_changed"));
	size_container->add_child(size_y_input);
	
	// Size Z
	size_z_input = memnew(LineEdit);
	size_z_input->set_text("10.0");
	size_z_input->set_custom_minimum_size(Vector2(60, 0));
	size_z_input->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	size_z_input->add_theme_font_size_override("font_size", 12);
	size_z_input->connect("text_changed", Callable(this, "on_size_z_changed"));
	size_container->add_child(size_z_input);
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
	file_dialog->popup_centered(Vector2i(800, 600));
}

void GrammarEditor::on_file_selected(String path) {
	UtilityFunctions::print("Loading file: ", path);
	
	// Update UI
	selected_file_label->set_text(path.get_file());
	selected_file_label->add_theme_color_override("font_color", Color(1, 1, 1));
	selected_file_path = path;
	
	// Initialize PMUGG with the selected file
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (editor_interface) {
        // Get current values
        seed_value = (int)seed_input->get_text().to_float();
        size_x_value = (float)size_x_input->get_text().to_float();
        size_y_value = (float)size_y_input->get_text().to_float();
        size_z_value = (float)size_z_input->get_text().to_float();
        
        // Initialize the grammar.
		const char* path_cstr = path.utf8().get_data();
		char result[1024];
		initialize(path_cstr, result, sizeof(result), seed_value);
		UtilityFunctions::print(result);
		
		// Set the size
		setSize(size_x_value, size_y_value, size_z_value);

        // Enable the buttons.
	    step_button->set_disabled(false);
	    reset_button->set_disabled(false);
	    play_button->set_disabled(false);
	} else {
		UtilityFunctions::print("No editor interface available");
	}
}

void GrammarEditor::on_step_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	iterate(1);
	update_mesh();
}

void GrammarEditor::on_reset_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	// Get current values
	seed_value = (int)seed_input->get_text().to_float();
	size_x_value = (float)size_x_input->get_text().to_float();
	size_y_value = (float)size_y_input->get_text().to_float();
	size_z_value = (float)size_z_input->get_text().to_float();
	
	reset(seed_value);
	setSize(size_x_value, size_y_value, size_z_value);
	update_mesh();
}

void GrammarEditor::on_play_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	if (!is_playing) {
		// Start playing
		is_playing = true;
		play_button->set_text("Stop");
		
		// Create timer if it doesn't exist
		if (!animation_timer) {
			animation_timer = memnew(Timer);
			animation_timer->set_wait_time(0.001); // Run as fast as possible
			animation_timer->connect("timeout", Callable(this, "on_animation_timer_timeout"));
			add_child(animation_timer);
		}
		
		animation_timer->start();
	} else {
		// Stop playing
		is_playing = false;
		play_button->set_text("Play");
		
		if (animation_timer) {
			animation_timer->stop();
		}
	}
}

void GrammarEditor::on_animation_timer_timeout() {
	if (is_playing) {
		iterate(1);
		update_mesh();
	}
}

void GrammarEditor::update_mesh() {
	MeshCpp mesh = getMesh();
	
	// Get current scene.
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

	// Remove mesh if empty. Godot does not support empty meshes.
	if (mesh.numVertices == 0 || mesh.numTriangles == 0) {
		MeshInstance3D* existing_mesh = find_generated_mesh();
		if (existing_mesh) {
			existing_mesh->queue_free();
		}
		return;
	}
	
	// Find existing mesh instance or create a new one.
	MeshInstance3D* mesh_instance = find_generated_mesh();
	if (!mesh_instance) {
		mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_name("Generated Mesh");
		current_scene->add_child(mesh_instance);
		mesh_instance->set_owner(current_scene);
	}

	PackedVector3Array vertices;
	PackedInt32Array indices;
	PackedVector3Array normals;

	// Switch Y and Z coordinates.
	vertices.resize(mesh.numVertices);
	for (int i = 0; i < mesh.numVertices; i++) {
		vertices[i] = Vector3(mesh.positions[i * 3], mesh.positions[i * 3 + 2], mesh.positions[i * 3 + 1]);
	}
	normals.resize(mesh.numVertices);
	for (int i = 0; i < mesh.numVertices; i++) {
		normals[i] = Vector3(mesh.normals[i * 3], mesh.normals[i * 3 + 2], mesh.normals[i * 3 + 1]);
	}

	indices.resize(mesh.numTriangles * 3);
	for (int i = 0; i < mesh.numTriangles; i++) {
		// Reverse the orientation for each triangle.
		indices[i * 3] = mesh.triangles[i * 3];
		indices[i * 3 + 1] = mesh.triangles[i * 3 + 2];
		indices[i * 3 + 2] = mesh.triangles[i * 3 + 1];
	}
	
	// Create the mesh surface.
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_INDEX] = indices;
	arrays[Mesh::ARRAY_NORMAL] = normals;

	Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	mesh_instance->set_mesh(array_mesh);
}

MeshInstance3D* GrammarEditor::find_generated_mesh() {
	Node* current_scene = EditorInterface::get_singleton()->get_edited_scene_root();
	if (!current_scene) {
		return nullptr;
	}
	for (int i = 0; i < current_scene->get_child_count(); i++) {
		Node* child = current_scene->get_child(i);
		if (child->get_class() == "MeshInstance3D" && child->get_name().begins_with("Generated Mesh")) {
			return Object::cast_to<MeshInstance3D>(child);
		}
	}
	return nullptr;
}

void GrammarEditor::on_size_x_changed(String value) {
    size_x_value = value.to_float();
    setSize(size_x_value, size_y_value, size_z_value);
}

void GrammarEditor::on_size_y_changed(String value) {
    size_y_value = value.to_float();
    setSize(size_x_value, size_y_value, size_z_value);
}

void GrammarEditor::on_size_z_changed(String value) {
    size_z_value = value.to_float();
    setSize(size_x_value, size_y_value, size_z_value);
}
