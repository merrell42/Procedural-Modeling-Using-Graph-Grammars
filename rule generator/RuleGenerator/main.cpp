// main.cpp: Entry point. Subcommands:
//
//   (no args) | smoke      Default: matches `../primitives/square filled.json`
//                          against `../graph templates/graph_templates.json`
//                          via the upstream GenerateRules API.
//
//   roundtrip [<library>]  Imports a template library, exports it to a
//                          `.roundtrip.json` sibling, re-parses both, and
//                          compares as JSON. Pass = TemplateGraph import +
//                          toJson are inverses (proves boundaryId / position
//                          / edges all survive — the matcher only reads
//                          connections, so these fields would otherwise be
//                          silently broken).
//
//   match <primitives> [<library>]   Runs the matcher pipeline against an
//                          arbitrary primitives JSON.
//
//   extract [<meshesDir> <outDir>]   Converts every .obj in meshesDir to a
//                          primitives JSON in outDir. Defaults: the `meshes`
//                          and `mesh_primitives` directories next to this
//                          tool (found by walking up from cwd / the exe).
//
//   --help                 Print usage.

#include "pch.h"
#include "RuleGenerator.h"
#include "RoundtripCommand.h"
#include "../../cpp_version/mesh_extraction/mesh_to_grammar.h"

#include <filesystem>
#include <iostream>
#include <string>

using namespace std;

namespace {

const char* kDefaultLibrary = "../graph templates/house.json";
constexpr const char* kDefaultPrimitives = "../primitives/house.json";

// constexpr const char* kDefaultLibrary = "../graph templates/graph_template_diagonal.json";
// constexpr const char* kDefaultPrimitives = "../primitives/diagonal box.json";

// constexpr const char* kDefaultLibrary = "../graph templates/graph_templates.json";
// constexpr const char* kDefaultPrimitives = "../primitives/square hollow.json";

constexpr const char* kDefaultOutput = "../generatedRules.json";

// Walk up from `start` until a directory containing `meshes/` is found.
std::filesystem::path findRuleGeneratorRootFrom(std::filesystem::path start) {
    std::error_code ec;
    for (std::filesystem::path dir = std::move(start); ; dir = dir.parent_path()) {
        if (std::filesystem::is_directory(dir / "meshes", ec)) return dir;
        std::filesystem::path parent = dir.parent_path();
        if (parent == dir) break;
    }
    return {};
}

std::filesystem::path findRuleGeneratorRoot(const char* exeArg) {
    std::filesystem::path root = findRuleGeneratorRootFrom(std::filesystem::current_path());
    if (!root.empty()) return root;
    std::error_code ec;
    std::filesystem::path exe = std::filesystem::absolute(exeArg, ec);
    if (ec) return {};
    return findRuleGeneratorRootFrom(exe.parent_path());
}

int usage(const char* exe) {
    cerr << "Usage:\n"
         << "  " << exe << "                              # default smoke\n"
         << "  " << exe << " smoke                        # default smoke\n"
         << "  " << exe << " roundtrip [<library.json>]   # TemplateGraph round-trip test\n"
         << "  " << exe << " match <primitives.json> [<library.json>]  # alternate input\n"
         << "  " << exe << " extract [<meshesDir> <outDir>]  # obj -> primitives JSON\n";
    return 2;
}

}  // namespace

int main(int argc, char* argv[]) {
    const char* exe = argc > 0 ? argv[0] : "RuleGenerator.exe";
    string cmd = (argc >= 2) ? argv[1] : "";

    if (cmd.empty() || cmd == "smoke") {
        return GenerateRules(kDefaultPrimitives, kDefaultLibrary, kDefaultOutput);
    }
    if (cmd == "roundtrip") {
        string libPath = (argc >= 3) ? argv[2] : kDefaultLibrary;
        return runRoundtrip(libPath);
    }
    if (cmd == "match") {
        if (argc < 3) return usage(exe);
        string primPath = argv[2];
        string libPath  = (argc >= 4) ? argv[3] : kDefaultLibrary;
        return GenerateRules(primPath, libPath, kDefaultOutput);
    }
    if (cmd == "extract") {
        if (argc == 3) return usage(exe);
        string meshesDir;
        string outDir;
        if (argc >= 4) {
            meshesDir = argv[2];
            outDir = argv[3];
        } else {
            auto root = findRuleGeneratorRoot(exe);
            if (root.empty()) {
                cerr << "Could not find a meshes directory. Pass <meshesDir> <outDir>.\n";
                return 1;
            }
            meshesDir = (root / "meshes").string();
            outDir = (root / "mesh_primitives").string();
        }
        return mesh_extraction::meshesToPrimitives(meshesDir.c_str(), outDir.c_str());
    }
    if (cmd == "--help" || cmd == "-h") {
        usage(exe);
        return 0;
    }
    return usage(exe);
}
