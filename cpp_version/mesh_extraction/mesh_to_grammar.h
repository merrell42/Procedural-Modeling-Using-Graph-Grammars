#pragma once

namespace mesh_extraction {

// Load an OBJ from `objPath`, build a half-edge mesh, infer vertex/edge/face
// types, and write a pmugg seed grammar to `outPath`. The output is a grammar
// JSON with a single starter rule that emits the mesh shape from the empty
// graph; it is intended for GraphGrammar::import() and the normal pmugg
// grammar pipeline.
//
// Return value: 0 on success; 1 on failure; 2 if the file was
// written but could not be re-imported.
int meshToGrammar(const char* objPath, const char* outPath);

}  // namespace mesh_extraction
