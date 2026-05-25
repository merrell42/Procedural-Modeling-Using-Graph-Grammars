#pragma once
// Serialize the half-edge mesh and its extracted types as a pmugg
// grammar JSON containing a single starter rule that emits the
// seed shape from the empty graph.
//
// The output is loadable by GraphGrammar::import() — i.e. it round-trips
// through loadGrammarCached() and can be passed to runGrammarEngine().

#include "mesh_extraction/half_edge_mesh.h"
#include "mesh_extraction/type_extractor.h"

#include <string>

namespace mesh_extraction {

// Writes a grammar JSON to `outPath`. Returns false (and sets `error`) on
// I/O failure.
bool writeSeedGrammar(const HalfEdgeMesh&    mesh,
                      const ExtractedTypes&  types,
                      const std::string&     name,
                      const std::string&     outPath,
                      std::string*           error = nullptr);

}  // namespace mesh_extraction
