#include "pch.h"
#include "RegressionCommand.h"
#include "RuleGenerator.h"

#include "third_party/json.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Json = nlohmann::json;
using namespace std;

namespace {

struct Case {
    const char* name;
    const char* library;     // relative to the rule generator root
    const char* primitives;
};

// Recorded from the generator before getTwin tolerated a one-sided edge.
const Case kCases[] = {
    {"graph_templates.square_hollow",
     "graph templates/graph_templates.json", "primitives/square hollow.json"},
    {"graph_templates.square_filled",
     "graph templates/graph_templates.json", "primitives/square filled.json"},
    {"L_template.platform-cross",
     "graph templates/L template.json", "mesh_primitives/platform-cross.json"},
    {"house.house",
     "graph templates/house.json", "mesh_primitives/house.json"},
    {"house.house2",
     "graph templates/house.json", "mesh_primitives/house2.json"},
    {"ramp.ramp",
     "graph templates/ramp.json", "mesh_primitives/ramp.json"},
};

filesystem::path findRootFrom(filesystem::path start) {
    error_code ec;
    for (filesystem::path dir = std::move(start); ; dir = dir.parent_path()) {
        if (filesystem::is_directory(dir / "meshes", ec)) return dir;
        filesystem::path parent = dir.parent_path();
        if (parent == dir) break;
    }
    return {};
}

filesystem::path findRoot(const char* exeArg) {
    filesystem::path root = findRootFrom(filesystem::current_path());
    if (!root.empty()) return root;
    error_code ec;
    filesystem::path exe = filesystem::absolute(exeArg ? exeArg : "", ec);
    if (ec) return {};
    return findRootFrom(exe.parent_path());
}

Json readJson(const filesystem::path& path) {
    ifstream in(path);
    if (!in) throw runtime_error("cannot open " + path.string());
    stringstream buf;
    buf << in.rdbuf();
    return Json::parse(buf.str());
}

bool isGraph(const Json& j) {
    return j.is_object()
        && j.contains("interior") && j["interior"].is_object()
        && j["interior"].contains("faces") && j["interior"]["faces"].is_array()
        && j.contains("faceTypes") && j["faceTypes"].is_array();
}

// Face creation follows unordered iteration of face types, so two runs of the
// same grammar can list the same faces in a different order. Sort each face
// by its type and the half-edges that use it, and rewrite face indices.
void canonicalizeGraph(Json& graph) {
    Json& faces = graph["interior"]["faces"];
    Json& faceTypes = graph["faceTypes"];
    const int n = (int)faces.size();
    vector<vector<int>> members(n);
    if (graph["interior"].contains("halfEdges") && graph["interior"]["halfEdges"].is_array()) {
        Json& halves = graph["interior"]["halfEdges"];
        for (int i = 0; i < (int)halves.size(); ++i) {
            const Json& half = halves[i];
            if (!half.is_object() || !half.contains("face") || !half["face"].is_number_integer()) continue;
            int face = half["face"].get<int>();
            if (face >= 0 && face < n) members[face].push_back(i);
        }
    }
    for (auto& member : members) sort(member.begin(), member.end());

    vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    sort(order.begin(), order.end(), [&](int a, int b) {
        int ta = (a < (int)faceTypes.size() && faceTypes[a].is_number_integer())
            ? faceTypes[a].get<int>() : -1;
        int tb = (b < (int)faceTypes.size() && faceTypes[b].is_number_integer())
            ? faceTypes[b].get<int>() : -1;
        if (ta != tb) return ta < tb;
        const auto& ma = members[a];
        const auto& mb = members[b];
        size_t count = ma.size() < mb.size() ? ma.size() : mb.size();
        for (size_t i = 0; i < count; ++i) {
            if (ma[i] != mb[i]) return ma[i] < mb[i];
        }
        if (ma.size() != mb.size()) return ma.size() < mb.size();
        return a < b;
    });

    vector<int> remap(n);
    for (int neu = 0; neu < n; ++neu) remap[order[neu]] = neu;

    Json newFaces = Json::array();
    Json newTypes = Json::array();
    for (int old : order) {
        newFaces.push_back(faces[old]);
        if (old < (int)faceTypes.size()) newTypes.push_back(faceTypes[old]);
    }
    faces = std::move(newFaces);
    faceTypes = std::move(newTypes);

    if (graph["interior"].contains("halfEdges") && graph["interior"]["halfEdges"].is_array()) {
        for (auto& half : graph["interior"]["halfEdges"]) {
            if (!half.is_object() || !half.contains("face") || !half["face"].is_number_integer()) continue;
            int face = half["face"].get<int>();
            if (face >= 0 && face < n) half["face"] = remap[face];
        }
    }
    if (graph.contains("morphism") && graph["morphism"].is_object()
        && graph["morphism"].contains("faces") && graph["morphism"]["faces"].is_array()) {
        for (auto& face : graph["morphism"]["faces"]) {
            if (!face.is_number_integer()) continue;
            int index = face.get<int>();
            if (index >= 0 && index < n) face = remap[index];
        }
    }
}

void canonicalize(Json& j) {
    if (j.is_array()) {
        for (auto& element : j) canonicalize(element);
        return;
    }
    if (!j.is_object()) return;
    if (isGraph(j)) canonicalizeGraph(j);
    for (auto& element : j.items()) canonicalize(element.value());
}

string firstDifference(const Json& a, const Json& b, const string& path) {
    if (a.type() != b.type()) return path + " type";
    if (a.is_array()) {
        if (a.size() != b.size()) {
            return path + " length " + to_string(a.size()) + " vs " + to_string(b.size());
        }
        for (size_t i = 0; i < a.size(); ++i) {
            string nested = firstDifference(a[i], b[i], path + "[" + to_string(i) + "]");
            if (!nested.empty()) return nested;
        }
        return {};
    }
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) {
            if (!b.contains(it.key())) return path + "." + it.key() + " missing from new grammar";
        }
        for (auto it = b.begin(); it != b.end(); ++it) {
            if (!a.contains(it.key())) return path + "." + it.key() + " missing from recorded grammar";
        }
        for (auto it = a.begin(); it != a.end(); ++it) {
            string nested = firstDifference(it.value(), b[it.key()], path + "." + it.key());
            if (!nested.empty()) return nested;
        }
        return {};
    }
    if (a != b) return path + " value";
    return {};
}

}  // namespace

int runRegression(const char* exeArg) {
    filesystem::path root = findRoot(exeArg);
    if (root.empty()) {
        cerr << "Could not find the rule generator root.\n";
        return 1;
    }
    filesystem::path recordedDir = root / "regression";
    int failed = 0;
    int checked = 0;
    for (const Case& item : kCases) {
        ++checked;
        filesystem::path recorded = recordedDir / (string(item.name) + ".json");
        filesystem::path actual = recordedDir / (string(item.name) + ".actual.json");
        cout << item.name << "\n";
        if (!filesystem::exists(recorded)) {
            cerr << "  FAIL missing recorded grammar " << recorded.string() << "\n";
            ++failed;
            continue;
        }
        stringstream captured;
        streambuf* previous = cout.rdbuf(captured.rdbuf());
        int code = GenerateRules(
            (root / item.primitives).string(),
            (root / item.library).string(),
            actual.string());
        cout.rdbuf(previous);
        if (code != 0) {
            cerr << "  FAIL generator exit " << code << "\n";
            ++failed;
            continue;
        }
        try {
            Json expected = readJson(recorded);
            Json got = readJson(actual);
            canonicalize(expected);
            canonicalize(got);
            if (expected == got) {
                cout << "  PASS\n";
                filesystem::remove(actual);
            } else {
                cerr << "  FAIL " << firstDifference(expected, got, "$") << "\n";
                cerr << "  actual: " << actual.string() << "\n";
                ++failed;
            }
        } catch (const exception& e) {
            cerr << "  FAIL " << e.what() << "\n";
            ++failed;
        }
    }
    cout << (checked - failed) << " of " << checked << " grammars match the recorded output\n";
    return failed == 0 ? 0 : 1;
}
