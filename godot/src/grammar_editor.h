#ifndef GRAMMAR_EDITOR_H
#define GRAMMAR_EDITOR_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/file_dialog.hpp>

// Include the DLL header directly
#include "../cpp_version/pmugg dll/generate.h"
#include "grammar_dock.h"

namespace godot {

class GrammarEditor : public EditorPlugin {
	GDCLASS(GrammarEditor, EditorPlugin)

private:
	bool pmugg_initialized;
	MeshInstance3D* mesh_instance;
	GrammarDock* dock;

protected:
	static void _bind_methods();

public:
	GrammarEditor();
	~GrammarEditor();

	virtual void _enter_tree() override;
	virtual void _exit_tree() override;
	
	// PMUGG functions for editor use
	void initialize_pmugg_editor(String file_path, int seed);
	void reset_pmugg_editor(int seed);
	void iterate_pmugg_editor(int steps);
	int get_face_count_editor();
	void set_pmugg_size_editor(float x, float y, float z);
	
	// Dock access
	GrammarDock* get_dock() { return dock; }
};

}

#endif 