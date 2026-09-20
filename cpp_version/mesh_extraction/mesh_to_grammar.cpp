#include "pch.h"
#include "mesh_extraction/mesh_to_grammar.h"

#include "graph_grammar.h"
#include "json versioning/read_json_file.h"
#include "mesh_extraction/half_edge_mesh.h"
#include "mesh_extraction/obj_loader.h"
#include "mesh_extraction/seed_grammar_writer.h"
#include "mesh_extraction/type_extractor.h"
#include "primitives/primitives.h"
#include "third_party/json.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>

namespace mesh_extraction {

namespace {

bool loadAndExtract(const char* objPath, HalfEdgeMesh& hem, ExtractedTypes& types,
                    std::string* err) {
	ObjMesh obj;
	if (!loadObj(objPath, obj, err)) {
		std::cerr << "loadObj failed: " << (err ? *err : "") << std::endl;
		return false;
	}
	if (!buildHalfEdgeMesh(obj, hem, /*useGroupAsVolume*/ false, err)) {
		std::cerr << "buildHalfEdgeMesh failed: " << (err ? *err : "") << std::endl;
		return false;
	}
	TypeExtractionConfig tcfg;
	if (!extractTypes(hem, obj, tcfg, types, err)) {
		std::cerr << "extractTypes failed: " << (err ? *err : "") << std::endl;
		return false;
	}
	return true;
}

std::string fileStem(const char* path) {
	return std::filesystem::path(path).stem().string();
}

bool isObjFile(const std::filesystem::path& path) {
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
	               [](unsigned char c) { return (char)std::tolower(c); });
	return ext == ".obj";
}

}  // namespace

int meshToGrammar(const char* objPath, const char* outPath) {
	HalfEdgeMesh hem;
	ExtractedTypes types;
	std::string err;
	if (!loadAndExtract(objPath, hem, types, &err)) {
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

int meshToPrimitives(const char* objPath, const char* outPath) {
	HalfEdgeMesh hem;
	ExtractedTypes types;
	std::string err;
	if (!loadAndExtract(objPath, hem, types, &err)) {
		return 1;
	}
	const std::string name = fileStem(objPath);
	if (!writePrimitives(types, name, outPath, &err)) {
		std::cerr << "writePrimitives failed: " << err << std::endl;
		return 1;
	}
	std::cout << "Wrote primitives to " << outPath
	          << " (" << types.vertexTypes.size() << " vertex types, "
	          << types.edgeTypes.size() << " edge types, "
	          << types.faceTypes.size() << " face types)" << std::endl;

	try {
		nlohmann::json parsed = readJsonFile(outPath, false);
		auto* primitives = Primitives::import(parsed["types"]);
		std::cout << "  Round-trip: Primitives::import succeeded." << std::endl;
		delete primitives;
	} catch (const std::exception& e) {
		std::cerr << "  Round-trip: Primitives::import threw: " << e.what() << std::endl;
		return 2;
	}
	return 0;
}

int meshesToPrimitives(const char* meshesDir, const char* outDir) {
	namespace fs = std::filesystem;
	const fs::path inPath(meshesDir);
	const fs::path outPath(outDir);
	if (!fs::exists(inPath) || !fs::is_directory(inPath)) {
		std::cerr << "not a directory: " << meshesDir << std::endl;
		return 1;
	}
	std::error_code ec;
	fs::create_directories(outPath, ec);
	if (ec) {
		std::cerr << "could not create " << outDir << ": " << ec.message() << std::endl;
		return 1;
	}

	int count = 0;
	int failures = 0;
	for (const auto& entry : fs::directory_iterator(inPath)) {
		if (!entry.is_regular_file()) continue;
		const fs::path& objPath = entry.path();
		if (!isObjFile(objPath)) continue;
		fs::path jsonPath = outPath / objPath.stem();
		jsonPath.replace_extension(".json");
		std::cout << objPath.filename().string() << " -> " << jsonPath.string() << std::endl;
		if (meshToPrimitives(objPath.string().c_str(), jsonPath.string().c_str()) != 0) {
			++failures;
		}
		++count;
	}
	if (count == 0) {
		std::cerr << "no .obj files in " << meshesDir << std::endl;
		return 1;
	}
	if (failures) {
		std::cerr << failures << " of " << count << " mesh(es) failed." << std::endl;
		return 1;
	}
	std::cout << "Extracted primitives for " << count << " mesh(es) to " << outDir << std::endl;
	return 0;
}

}  // namespace mesh_extraction
