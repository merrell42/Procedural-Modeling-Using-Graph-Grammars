#ifndef GRAMMAR_EDITOR_H
#define GRAMMAR_EDITOR_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

// Include the DLL header directly
#include "../cpp_version/pmugg dll/generate.h"

namespace godot {

class GrammarEditor : public EditorPlugin {
	GDCLASS(GrammarEditor, EditorPlugin)

private:
	MeshInstance3D* mesh_instance;
	
	// UI elements
	Button* load_grammar_button;
	Button* load_folder_button;
	Button* reset_button;
	Button* step_button;
	Button* play_button;
	Label* selected_file_label;
	Label* iteration_label;
	LineEdit* seed_input;
	LineEdit* size_x_input;
	LineEdit* size_y_input;
	LineEdit* size_z_input;
	FileDialog* file_dialog;
	FileDialog* folder_dialog;
	String selected_file_path;
	
	// Animation state
	bool is_playing;
	Timer* animation_timer;

	// Values
	int seed_value;
	float size_x_value;
	float size_y_value;
	float size_z_value;
	int iteration_count;
	int max_iterations;
	
	// Folder processing
	PackedStringArray folder_files;
	int current_file_index;

	// Callbacks
	void on_load_grammar_pressed();
	void on_load_folder_pressed();
	void on_file_selected(String path);
	void on_folder_selected(String path);
	void on_reset_pressed();
	void on_step_pressed();
	void on_play_pressed();
	void on_animation_timer_timeout();
	void on_size_x_changed(String value);
	void on_size_y_changed(String value);
	void on_size_z_changed(String value);
	
	// Helper methods
	void load_file_from_folder(String file_path);
	void update_iteration_display();
	void find_json_files_recursive(String folder_path);

protected:
	static void _bind_methods();

public:
	GrammarEditor();
	~GrammarEditor();

	virtual void _enter_tree() override;
	virtual void _exit_tree() override;
	
	// UI setup
	void setup_ui(Control* parent);
	void setup_file_dialog(Control* parent);
	
	void update_mesh();
	MeshInstance3D* find_generated_mesh();
};

}

#endif 