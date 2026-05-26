#include "pch.h"
#include "mesh_extraction/mesh_to_grammar.h"

#include "graph_grammar.h"
#include "json versioning/read_json_file.h"
#include "mesh_extraction/half_edge_mesh.h"
#include "mesh_extraction/obj_loader.h"
#include "mesh_extraction/seed_grammar_writer.h"
#include "mesh_extraction/type_extractor.h"
#include "third_party/json.h"

#include <iostream>
#include <string>

namespace mesh_extraction {

int meshToGrammar(const char* objPath, const char* outPath) {
	ObjMesh obj;
	std::string err;
	if (!loadObj(objPath, obj, &err)) {
		std::cerr << "loadObj failed: " << err << std::endl;
		return 1;
	}
	HalfEdgeMesh hem;
	if (!buildHalfEdgeMesh(obj, hem, /*useGroupAsVolume*/ false, &err)) {
		std::cerr << "buildHalfEdgeMesh failed: " << err << std::endl;
		return 1;
	}
	ExtractedTypes types;
	TypeExtractionConfig tcfg;
	if (!extractTypes(hem, obj, tcfg, types, &err)) {
		std::cerr << "extractTypes failed: " << err << std::endl;
		return 1;
	}
	if (!writeSeedGrammar(hem, types, "seed", outPath, &err)) {
		std::cerr << "writeSeedGrammar failed: " << err << std::endl;
		return 1;
	}
	std::cout << "Wrote seed grammar to " << outPath
	          << " (" << types.vertexTypes.size() << " vertex types, "
	          << types.edgeTypes.size() << " edge types, "
	          << types.faceTypes.size() << " face types)" << std::endl;

	// Sanity-check that the JSON we just wrote loads via the normal pmugg
	// grammar loader. The mutator isn't invoked here -- this is a parse check.
	try {
		nlohmann::json parsed = readJsonFile(outPath, false);
		auto* grammar = GraphGrammar::import(parsed);
		std::cout << "  Round-trip: GraphGrammar::import succeeded." << std::endl;
		delete grammar;
	} catch (const std::exception& e) {
		std::cerr << "  Round-trip: GraphGrammar::import threw: " << e.what() << std::endl;
		return 2;
	}
	return 0;
}

}  // namespace mesh_extraction
