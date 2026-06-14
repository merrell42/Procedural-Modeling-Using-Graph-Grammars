// main.cpp: Entry point. Three subcommands:
//
//   (no args) | smoke      Default: matches `../primitives/square filled.json`
//                          against `../graph templates/graph_templates.json`
//                          via the upstream GenerateRules API.
//
//   roundtrip [<library>]  Imports a template library, exports it to a
//                          `.roundtrip.json` sibling, re-parses both, and
//                          compares as JSON. Pass = TemplateGraph import +
//                          toJson are inverses (proves boundaryId / position
//                          / numEdges all survive — the matcher only reads
//                          connections, so these fields would otherwise be
//                          silently broken).
//
//   match <primitives> [<library>]   Runs the matcher pipeline against an
//                          arbitrary primitives JSON.
//
//   --help                 Print usage.

#include "pch.h"
#include "RuleGenerator.h"
#include "RoundtripCommand.h"

#include <iostream>
#include <string>

using namespace std;

namespace {


// constexpr const char* kDefaultLibrary = "../graph templates/graph_template_diagonal.json";
// constexpr const char* kDefaultPrimitives = "../primitives/diagonal box.json";

constexpr const char* kDefaultLibrary = "../graph templates/graph_templates.json";
constexpr const char* kDefaultPrimitives = "../primitives/square hollow.json";
constexpr const char* kDefaultOutput = "../generatedRules.json";

int usage(const char* exe) {
    cerr << "Usage:\n"
         << "  " << exe << "                              # default smoke\n"
         << "  " << exe << " smoke                        # default smoke\n"
         << "  " << exe << " roundtrip [<library.json>]   # TemplateGraph round-trip test\n"
         << "  " << exe << " match <primitives.json> [<library.json>]  # alternate input\n";
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
    if (cmd == "--help" || cmd == "-h") {
        usage(exe);
        return 0;
    }
    return usage(exe);
}
