#include "pch.h"
#include "RoundtripCommand.h"

#include "TemplateGraph.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using Json = nlohmann::json;

namespace {

// Loads a template library as raw JSON (independent of our TemplateGraph
// importer) for use as a round-trip baseline.
Json loadRawJson(const string& path) {
    ifstream in(path);
    if (!in) throw runtime_error("cannot open " + path);
    stringstream buf;
    buf << in.rdbuf();
    return Json::parse(buf.str());
}

}  // namespace

int runRoundtrip(const string& libPath) {
    try {
        // Import via our TemplateGraph reader, export to sibling file.
        auto sets = importTemplateGraphs(libPath);
        string outPath = libPath + ".roundtrip.json";
        if (!exportTemplateGraphs(outPath, sets)) {
            cerr << "FAIL: exportTemplateGraphs could not write " << outPath << endl;
            return 1;
        }

        // Parse both files as raw JSON. nlohmann::json::operator== is
        // structural and order-insensitive on object keys; arrays are
        // order-sensitive (which is correct for vertices / graphs).
        Json original = loadRawJson(libPath);
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
