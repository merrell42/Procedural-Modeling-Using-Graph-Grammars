// main.cpp: Entry point. Three subcommands so steps 0-3 of
// RULE_GENERATOR_PLAN.md can be verified independently:
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
//                          arbitrary primitives JSON. Used as the
//                          alternate-input sanity check (different primitives
//                          → different match counts).
//
//   --help                 Print usage.

#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using Json = nlohmann::json;

namespace {

constexpr const char* kDefaultLibrary = "../graph templates/graph_templates.json";
constexpr const char* kDefaultPrimitives = "../primitives/square filled.json";

int usage(const char* exe) {
    cerr << "Usage:\n"
         << "  " << exe << "                              # default smoke\n"
         << "  " << exe << " smoke                        # default smoke\n"
         << "  " << exe << " roundtrip [<library.json>]   # TemplateGraph round-trip test\n"
         << "  " << exe << " match <primitives.json> [<library.json>]  # alternate input\n";
    return 2;
}

int runSmoke() {
    char output[1024] = {};
    try {
        Json parsed = readJsonFile(kDefaultPrimitives, false);
        string jsonString = parsed.dump();
        GenerateRules(jsonString.c_str(), output, sizeof(output));
        cout << "Success: " << output << endl;
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}

// Loads a template library as raw JSON (independent of our TemplateGraph
// importer) for use as a round-trip baseline.
Json loadRawJson(const string& path) {
    ifstream in(path);
    if (!in) throw runtime_error("cannot open " + path);
    stringstream buf;
    buf << in.rdbuf();
    return Json::parse(buf.str());
}

int runRoundtrip(const string& libPath) {
    try {
        // 1. Import via our TemplateGraph reader, export to sibling file.
        auto sets = importTemplateGraphs(libPath);
        string outPath = libPath + ".roundtrip.json";
        if (!exportTemplateGraphs(outPath, sets)) {
            cerr << "FAIL: exportTemplateGraphs could not write " << outPath << endl;
            return 1;
        }

        // 2. Parse both files as raw JSON. nlohmann::json::operator== is
        //    structural and order-insensitive on object keys; arrays are
        //    order-sensitive (which is correct for vertices / graphs).
        Json original  = loadRawJson(libPath);
        Json roundtrip = loadRawJson(outPath);

        cout << "round-trip:\n"
             << "  library      : " << libPath << "\n"
             << "  roundtrip out: " << outPath << "\n"
             << "  entries      : " << sets.size() << "\n";

        // Field-level summary so a failure is easy to localize.
        if (original.is_array() && roundtrip.is_array()
                && original.size() == roundtrip.size()) {
            for (size_t i = 0; i < original.size(); ++i) {
                bool same = (original[i] == roundtrip[i]);
                cout << "  entry " << i << "      : "
                     << (same ? "MATCH" : "DIFF") << "\n";
                if (!same) {
                    // Dump a short diff hint (compact form, capped).
                    string a = original[i].dump();
                    string b = roundtrip[i].dump();
                    cout << "    original : " << a.substr(0, 200) << (a.size() > 200 ? "..." : "") << "\n";
                    cout << "    roundtrip: " << b.substr(0, 200) << (b.size() > 200 ? "..." : "") << "\n";
                }
            }
        } else {
            cout << "  shape        : DIFF (root must be same-length array)\n";
        }

        bool ok = (original == roundtrip);
        cout << "  result       : " << (ok ? "PASS" : "FAIL") << endl;
        return ok ? 0 : 1;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}

// Runs the matcher pipeline against the given primitives + library. Mirrors
// GenerateRules() but takes paths and prints per-entry match counts instead
// of bare match rows. Diverges slightly from upstream so failure modes are
// visible.
int runMatch(const string& primitivesPath, const string& libPath) {
    try {
        Json parsed = readJsonFile(primitivesPath, false);
        Primitives* primitives = Primitives::import(parsed["types"]);
        Json types = parsed["types"];

        // Tag edges/vertices with ruleGeneratorIds (same logic as GenerateRules).
        for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
            EdgeType* eType = primitives->edgeTypes[i];
            if (types["edgeTypes"].size() > i && types["edgeTypes"][i].contains("id")) {
                eType->setRuleGeneratorId(types["edgeTypes"][i]["id"].get<string>());
            } else {
                eType->setRuleGeneratorId("edge" + to_string(i));
            }
        }
        vector<VertexType*> vTypes;
        for (size_t i = 0; i < primitives->vertexTypes.size(); i++) {
            VertexType* vType = primitives->vertexTypes[i];
            if (types["vertexTypes"].size() > i && types["vertexTypes"][i].contains("id")) {
                vType->setRuleGeneratorId(types["vertexTypes"][i]["id"].get<int>());
            }
            vTypes.push_back(vType);
        }

        auto sets = importTemplateGraphs(libPath);

        cout << "match:\n"
             << "  primitives: " << primitivesPath << "\n"
             << "  library   : " << libPath << "\n"
             << "  entries   : " << sets.size() << "\n";

        size_t totalMatches = 0;
        for (size_t i = 0; i < sets.size(); ++i) {
            cout << "  [" << i << "] \"" << sets[i].comment << "\"  ";
            if (sets[i].graphs.empty()) {
                cout << "skipped (empty graphs)\n";
                continue;
            }
            vector<Json> matches;
            TemplateMatcher matcher(sets[i].graphs[0], vTypes, /*excludeRepeats*/ false);
            matcher.match();
            matcher.GetMatches(matches);
            cout << "matches=" << matches.size() << "\n";
            totalMatches += matches.size();
        }
        cout << "  total     : " << totalMatches << " match(es) across "
             << sets.size() << " entries" << endl;
        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const char* exe = argc > 0 ? argv[0] : "RuleGenerator.exe";
    string cmd = (argc >= 2) ? argv[1] : "";

    if (cmd.empty() || cmd == "smoke") {
        return runSmoke();
    }
    if (cmd == "roundtrip") {
        string libPath = (argc >= 3) ? argv[2] : kDefaultLibrary;
        return runRoundtrip(libPath);
    }
    if (cmd == "match") {
        if (argc < 3) return usage(exe);
        string primPath = argv[2];
        string libPath  = (argc >= 4) ? argv[3] : kDefaultLibrary;
        return runMatch(primPath, libPath);
    }
    if (cmd == "--help" || cmd == "-h") {
        usage(exe);
        return 0;
    }
    return usage(exe);
}
