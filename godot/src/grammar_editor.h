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
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// Include the DLL header directly
#include "../cpp_version/pmugg dll/generate.h"

namespace godot {

class GrammarEditor : public EditorPlugin {
	GDCLASS(GrammarEditor, EditorPlugin)

private:
	MeshInstance3D* mesh_instance;
	
	// UI elements
	Button* load_grammar_button;
	Button* step_button_ref;
	Label* selected_file_label;
	FileDialog* file_dialog;
	String selected_file_path;

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
	
	// Event handlers
	void on_load_grammar_pressed();
	void on_file_selected(String path);
	void on_step_pressed();

	void update_mesh();
	MeshInstance3D* find_generated_mesh();
};

}

#endif 