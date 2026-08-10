#include "grammar_editor.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

using namespace godot;

#define MAX_ITERATIONS 200

 // Animation time step per frame.
#define TIME_PER_FRAME 0.05f

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
    ClassDB::bind_method(D_METHOD("on_load_folder_pressed"), &GrammarEditor::on_load_folder_pressed);
    ClassDB::bind_method(D_METHOD("on_folder_selected"), &GrammarEditor::on_folder_selected);
    ClassDB::bind_method(D_METHOD("load_file_from_folder"), &GrammarEditor::load_file_from_folder);
    ClassDB::bind_method(D_METHOD("update_iteration_display"), &GrammarEditor::update_iteration_display);
    ClassDB::bind_method(D_METHOD("create_edge_lines"), &GrammarEditor::create_edge_lines);
}

GrammarEditor::GrammarEditor() {
    mesh_instance = nullptr;
    edge_lines_instance = nullptr;

    load_grammar_button = nullptr;
    step_button = nullptr;
    selected_file_label = nullptr;
    file_dialog = nullptr;
    selected_file_path = "";
    
    is_playing = false;
    animation_timer = nullptr;

    seed_value = 0;
    size_x_value = 30.0f;
    size_y_value = 20.0f;
    size_z_value = 10.0f;
    iteration_count = 0;
    max_iterations = 50;
    
    // The index of the file in the folder we are currently processing.
    current_file_index = 0;
}

GrammarEditor::~GrammarEditor() {}

void GrammarEditor::_enter_tree() {
    // Create a container for the UI.
    Control* ui_container = memnew(Control);
    ui_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    ui_container->set_custom_minimum_size(Vector2(200, 0));
    
    // Create the UI in the container.
    setup_ui(ui_container);
    setup_file_dialog(ui_container);
    
    // Add the UI container to a dock slot
    add_control_to_dock(DOCK_SLOT_LEFT_UL, ui_container);
}

void GrammarEditor::_exit_tree() {
    if (mesh_instance && mesh_instance->is_inside_tree()) {
        mesh_instance->queue_free();
    }
}

void GrammarEditor::setup_ui(Control* parent) {
	// Create main layout
	VBoxContainer* vbox = memnew(VBoxContainer);
	vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	parent->add_child(vbox);

	Label* title = memnew(Label);
	title->set_text("Graph Grammar Editor");
	title->add_theme_font_size_override("font_size", 16);
	vbox->add_child(title);
	
	// File selection section.
	VBoxContainer* file_section = memnew(VBoxContainer);
	file_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(file_section);
	
	// Currently selected file.
	selected_file_label = memnew(Label);
	selected_file_label->set_text("No file selected");
	selected_file_label->add_theme_color_override("font_color", Color(0.5, 0.5, 0.5));
	file_section->add_child(selected_file_label);
	
	load_grammar_button = create_button("Load Grammar...", "on_load_grammar_pressed", file_section);
	load_folder_button = create_button("Load Folder...", "on_load_folder_pressed", file_section);
	
	// Generation section
	VBoxContainer* gen_section = memnew(VBoxContainer);
	gen_section->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbox->add_child(gen_section);
	
	// Iteration display
	iteration_label = memnew(Label);
	iteration_label->set_text("Step: 0");
	iteration_label->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	gen_section->add_child(iteration_label);

    // Buttons.
	play_button = create_button("Play", "on_play_pressed", gen_section, true);
	reset_button = create_button("Reset", "on_reset_pressed", gen_section, true);
	step_button = create_button("Step", "on_step_pressed", gen_section, true);
	
	// Parameters section
	VBoxContainer* params_section = memnew(VBoxContainer);
	vbox->add_child(params_section);
	
	// Seed input
	HBoxContainer* seed_container = memnew(HBoxContainer);
	params_section->add_child(seed_container);
	
	Label* seed_label = memnew(Label);
	seed_label->set_text("Seed:");
	seed_label->set_custom_minimum_size(Vector2(50, 0));
	seed_container->add_child(seed_label);
	
	seed_input = create_input("0", "", seed_container);
	
	// Size inputs
	HBoxContainer* size_container = memnew(HBoxContainer);
	params_section->add_child(size_container);
	
	Label* size_label = memnew(Label);
	size_label->set_text("Size:");
	size_label->set_custom_minimum_size(Vector2(50, 0));
	size_container->add_child(size_label);

	size_x_input = create_input("30.0", "on_size_x_changed", size_container);
	size_y_input = create_input("20.0", "on_size_y_changed", size_container);
	size_z_input = create_input("10.0", "on_size_z_changed", size_container);
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
	
	// Setup folder dialog
	folder_dialog = memnew(FileDialog);
	parent->add_child(folder_dialog);
	
	folder_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	folder_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	
	if (DirAccess::dir_exists_absolute(grammar_dir)) {
		folder_dialog->set_current_dir(grammar_dir);
	} else {
		UtilityFunctions::print("Grammar data directory is not found at: ", grammar_dir);
		folder_dialog->set_current_dir(base_dir);
	}
	folder_dialog->connect("dir_selected", Callable(this, "on_folder_selected"));
}

void GrammarEditor::on_load_grammar_pressed() {
	file_dialog->popup_centered(Vector2i(800, 600));
}

void GrammarEditor::on_file_selected(String path) {
	UtilityFunctions::print("Loading file: ", path);
	
	// Update UI.
	selected_file_label->set_text(path.get_file());
	selected_file_label->add_theme_color_override("font_color", Color(1, 1, 1));
	selected_file_path = path;
	
	// Initialize PMUGG with the selected file.
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (editor_interface) {
		// Initialize the grammar.
		CharString char_str = path.utf8();
		const char* path_cstr = char_str.get_data();
		char result[1024];
		get_current_values();
		initialize(path_cstr, result, sizeof(result), seed_value);
		UtilityFunctions::print(result);
		
		// Set the size.
		setSize(size_x_value, size_y_value, size_z_value);

        // Enable the buttons.
	    step_button->set_disabled(false);
	    reset_button->set_disabled(false);
	    play_button->set_disabled(false);
        max_iterations = -1;
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
	iteration_count++;
	update_iteration_display();
	update_mesh();
}

void GrammarEditor::on_reset_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	get_current_values();
	reset(seed_value);
	setSize(size_x_value, size_y_value, size_z_value);
	
	iteration_count = 0;
	update_iteration_display();
	update_mesh();
}

void GrammarEditor::on_play_pressed() {
	if (selected_file_path.is_empty()) {
		UtilityFunctions::print("No file selected!");
		return;
	}
	
	if (!is_playing) {
		start_animation();
	} else {
		stop_animation();
	}
}

void GrammarEditor::on_animation_timer_timeout() {
    if (is_playing) {
		// TODO: Check that this is working. I'm having trouble recompiling the plugin.
        int steps_completed = iterateToTime(TIME_PER_FRAME);
        iteration_count += steps_completed;
        update_iteration_display();
        update_mesh();
        
        // Check if we've reached max iterations.
        if (iteration_count >= max_iterations && max_iterations > 0) {
            // Move to next file if available.
            if (current_file_index < folder_files.size() - 1) {
                current_file_index++;
                load_file_from_folder(folder_files[current_file_index]);
            } else {
                // Stop playing once we've reached the end of the folder.
                stop_animation();
                UtilityFunctions::print("Finished processing all files in folder");
            }
        }
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

	// Check if we have any renderable geometry.
	bool has_mesh = false;
	bool has_lines = false;
	for (int submesh_index = 0; submesh_index < mesh.numSubmeshes; submesh_index++) {
		SubmeshCpp submesh = mesh.submeshes[submesh_index];
		if (submesh.numVertices > 0 && submesh.numTriangles > 0) {
			has_mesh = true;
		}
		if (submesh.numVertices > 0 && submesh.numFaces > 0) {
			has_lines = true;
		}
	}

	if (!has_mesh && !has_lines) {
		if (mesh_instance) {
			mesh_instance->queue_free();
			mesh_instance = nullptr;
		}
		
		if (edge_lines_instance) {
			edge_lines_instance->queue_free();
			edge_lines_instance = nullptr;
		}
		destroyMesh(mesh);
		return;
	}
	
	if (has_mesh) {
		if (!mesh_instance) {
			mesh_instance = memnew(MeshInstance3D);
			mesh_instance->set_name("Generated Mesh");
			current_scene->add_child(mesh_instance);
			mesh_instance->set_owner(current_scene);
		}

		Ref<ArrayMesh> array_mesh = memnew(ArrayMesh);
		
		for (int submesh_index = 0; submesh_index < mesh.numSubmeshes; submesh_index++) {
			SubmeshCpp submesh = mesh.submeshes[submesh_index];
			
			if (submesh.numVertices == 0 || submesh.numTriangles == 0) {
				continue;
			}

			PackedVector3Array vertices;
			PackedInt32Array indices;
			PackedVector3Array normals;

			// Switch Y and Z coordinates.
			vertices.resize(submesh.numVertices);
			for (int i = 0; i < submesh.numVertices; i++) {
				vertices[i] = Vector3(submesh.positions[i * 3], submesh.positions[i * 3 + 2], submesh.positions[i * 3 + 1]);
			}
			normals.resize(submesh.numVertices);
			for (int i = 0; i < submesh.numVertices; i++) {
				normals[i] = Vector3(submesh.normals[i * 3], submesh.normals[i * 3 + 2], submesh.normals[i * 3 + 1]);
			}

			indices.resize(submesh.numTriangles * 3);
			for (int i = 0; i < submesh.numTriangles; i++) {
				indices[i * 3] = submesh.triangles[i * 3];
				indices[i * 3 + 1] = submesh.triangles[i * 3 + 2];
				indices[i * 3 + 2] = submesh.triangles[i * 3 + 1];
			}

			// Create the mesh surface for this submesh.
			Array arrays;
			arrays.resize(Mesh::ARRAY_MAX);
			arrays[Mesh::ARRAY_VERTEX] = vertices;
			arrays[Mesh::ARRAY_INDEX] = indices;
			arrays[Mesh::ARRAY_NORMAL] = normals;

			array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
			
			// Create material for this submesh based on color.
			Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
			material->set_albedo(Color(submesh.red, submesh.green, submesh.blue, 1.0f));
			array_mesh->surface_set_material(submesh_index, material);
		}
		
		mesh_instance->set_mesh(array_mesh);
	} else if (mesh_instance) {
		mesh_instance->queue_free();
		mesh_instance = nullptr;
	}
	
	// Create edge lines from the first submesh (for visualization).
	if (has_lines && mesh.numSubmeshes > 0) {
		SubmeshCpp first_submesh = mesh.submeshes[0];
		if (first_submesh.numVertices > 0 && first_submesh.numFaces > 0) {
			PackedVector3Array vertices;
			PackedInt32Array face_indices;
			
			// Convert vertices for edge lines.
			vertices.resize(first_submesh.numVertices);
			for (int i = 0; i < first_submesh.numVertices; i++) {
				vertices[i] = Vector3(first_submesh.positions[i * 3], first_submesh.positions[i * 3 + 2], first_submesh.positions[i * 3 + 1]);
			}
			
			// Convert face indices for edge lines.
			face_indices.resize(first_submesh.numFaces);
			for (int i = 0; i < first_submesh.numFaces; i++) {
				face_indices[i] = first_submesh.faceIndices[i];
			}
			
			create_edge_lines(vertices, face_indices);
		}
	} else if (edge_lines_instance) {
		edge_lines_instance->queue_free();
		edge_lines_instance = nullptr;
	}
	
	// Clean up the mesh data.
	destroyMesh(mesh);
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

void GrammarEditor::on_load_folder_pressed() {
	folder_dialog->popup_centered(Vector2i(800, 600));
}

void GrammarEditor::on_folder_selected(String path) {
	UtilityFunctions::print("Loading folder: ", path);

	folder_files.clear();
	current_file_index = 0;
	find_json_files(path);
	
	if (folder_files.size() > 0) {
		UtilityFunctions::print("Found ", folder_files.size(), " JSON files in the folder.");
        // Play the first file.
		load_file_from_folder(folder_files[0]);
		start_animation();
		max_iterations = MAX_ITERATIONS;
	} else {
		UtilityFunctions::print("No JSON files found in folder or subfolders");
	}
}

// Recursively get all JSON files in the folder and subfolders.
void GrammarEditor::find_json_files(String folder_path) {
	Ref<DirAccess> dir = DirAccess::open(folder_path);
	if (!dir.is_valid()) {
		return;
	}
	
	dir->list_dir_begin();
	String file_name = dir->get_next();
	
	while (!file_name.is_empty()) {
		if (file_name == "." || file_name == "..") {
			// Skip current and parent directory
		} else if (dir->current_is_dir()) {
			// Recursively search subdirectories
			String subfolder_path = folder_path.path_join(file_name);
			find_json_files(subfolder_path);
		} else if (file_name.ends_with(".json")) {
			// Add JSON file to the list
			folder_files.append(folder_path.path_join(file_name));
		}
		file_name = dir->get_next();
	}
	dir->list_dir_end();
}

void GrammarEditor::load_file_from_folder(String file_path) {
	UtilityFunctions::print("Loading file from folder: ", file_path);
	
	// Update UI.
	selected_file_label->set_text(file_path.get_file());
	selected_file_label->add_theme_color_override("font_color", Color(1, 1, 1));
	selected_file_path = file_path;
	
	// Reset iteration count.
	iteration_count = 0;
	update_iteration_display();
	

    // Initialize the grammar.
    const char* path_cstr = file_path.utf8().get_data();
    char result[1024];
    get_current_values();
    initialize(path_cstr, result, sizeof(result), seed_value);
    UtilityFunctions::print(result);
    setSize(size_x_value, size_y_value, size_z_value);

    // Enable the buttons.
    step_button->set_disabled(false);
    reset_button->set_disabled(false);
    play_button->set_disabled(false);
}

void GrammarEditor::update_iteration_display() {
    if (max_iterations > 0) {
        iteration_label->set_text("Step: " + String::num_int64(iteration_count) + " / " + String::num_int64(max_iterations));
    } else {
        iteration_label->set_text("Step: " + String::num_int64(iteration_count));
    }
}

void GrammarEditor::add_line(PackedVector3Array& edge_vertices, PackedInt32Array& edge_indices, const Vector3& v1, const Vector3& v2) {
	edge_vertices.append(v1);
	edge_vertices.append(v2);
	edge_indices.append(edge_vertices.size() - 2);
	edge_indices.append(edge_vertices.size() - 1);
}

void GrammarEditor::create_edge_lines(const PackedVector3Array& vertices, const PackedInt32Array& face_indices) {
	// Get current scene.
	EditorInterface* editor_interface = EditorInterface::get_singleton();
	if (!editor_interface) {
		return;
	}
	Node* current_scene = editor_interface->get_edited_scene_root();
	if (!current_scene) {
		return;
	}
	
	// Find existing edge lines instance or create new one.
	if (!edge_lines_instance) {
		edge_lines_instance = memnew(MeshInstance3D);
		edge_lines_instance->set_name("Edge Lines");
		current_scene->add_child(edge_lines_instance);
		edge_lines_instance->set_owner(current_scene);
	}

	PackedVector3Array edge_vertices;
	PackedInt32Array edge_indices;

	int start_index = 0;
	for (int i = 0; i < face_indices.size(); i++) {
		int end_index = face_indices[i];
		for (int j = start_index; j < end_index - 1; j++) {
			add_line(edge_vertices, edge_indices, vertices[j], vertices[j + 1]);
		}
		add_line(edge_vertices, edge_indices, vertices[end_index - 1], vertices[start_index]);
		start_index = end_index;
	}
	
	// Create the edge mesh.
	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = edge_vertices;
	arrays[Mesh::ARRAY_INDEX] = edge_indices;
	
	Ref<ArrayMesh> edge_mesh = memnew(ArrayMesh);
	edge_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	edge_lines_instance->set_mesh(edge_mesh);
	
	// Create a material for the edge lines.
	Ref<StandardMaterial3D> edge_material = memnew(StandardMaterial3D);
	edge_material->set_albedo(Color(1, 0, 0, 1.0)); // Red color for edges
	edge_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	edge_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	edge_lines_instance->set_material_override(edge_material);
}

Button* GrammarEditor::create_button(const String& text, const String& callback, Control* parent, bool disabled) {
	Button* button = memnew(Button);
	button->set_text(text);
	button->set_disabled(disabled);
	button->connect("pressed", Callable(this, callback));
	button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	button->add_theme_font_size_override("font_size", 14);
	parent->add_child(button);
	return button;
}

LineEdit* GrammarEditor::create_input(const String& default_value, const String& callback, Control* parent, float min_width) {
	LineEdit* input = memnew(LineEdit);
	input->set_text(default_value);
	input->set_custom_minimum_size(Vector2(min_width, 0));
	input->add_theme_color_override("font_color", Color(0.7, 0.7, 0.7));
	input->add_theme_font_size_override("font_size", 12);
	if (!callback.is_empty()) {
		input->connect("text_changed", Callable(this, callback));
	}
	parent->add_child(input);
	return input;
}

void GrammarEditor::get_current_values() {
	seed_value = (int)seed_input->get_text().to_float();
	size_x_value = (float)size_x_input->get_text().to_float();
	size_y_value = (float)size_y_input->get_text().to_float();
	size_z_value = (float)size_z_input->get_text().to_float();
}

void GrammarEditor::start_animation() {
    is_playing = true;
    play_button->set_text("Stop");

	// Create timer if it doesn't exist.
	if (!animation_timer) {
		animation_timer = memnew(Timer);
		animation_timer->set_wait_time(0.001);
		animation_timer->connect("timeout", Callable(this, "on_animation_timer_timeout"));
		add_child(animation_timer);
	}
	
	animation_timer->start();
}

void GrammarEditor::stop_animation() {
    is_playing = false;
    play_button->set_text("Play");

    if (animation_timer) {
        animation_timer->stop();
    }
}