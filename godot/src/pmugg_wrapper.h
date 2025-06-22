#ifndef PMUGG_WRAPPER_H
#define PMUGG_WRAPPER_H

// Include the DLL header
#include "pmugg dll/generate.h"

namespace godot {

class PMUGGWrapper {
public:
    static void initialize_grammar(const char* filePath, char* result, int len, int seed);
    static void reset_grammar(int seed);
    static void iterate_grammar(int steps);
    static int get_num_faces();
    static MeshCpp get_mesh();
    static void set_size(float x, float y, float z);
    static void destroy_mesh(MeshCpp& mesh);
};

}

#endif 