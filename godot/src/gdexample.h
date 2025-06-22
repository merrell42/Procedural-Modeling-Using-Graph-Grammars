#ifndef GDEXAMPLE_H
#define GDEXAMPLE_H

#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include "pmugg_wrapper.h"

namespace godot {

class GDExample : public Sprite2D {
	GDCLASS(GDExample, Sprite2D)

private:
	double time_passed;
	bool pmugg_initialized;

protected:
	static void _bind_methods();

public:
	GDExample();
	~GDExample();

	void _process(double delta) override;
	
	// Expose DLL functions to Godot
	void initialize_pmugg(String file_path, int seed);
	void reset_pmugg(int seed);
	void iterate_pmugg(int steps);
	int get_face_count();
	void set_pmugg_size(float x, float y, float z);
};

}

#endif