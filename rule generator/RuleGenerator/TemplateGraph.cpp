#include "pch.h"
#include "TemplateGraph.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

TemplateVertex TemplateVertex::import(const Json& j) {
    TemplateVertex v;
    if (j.contains("connections") && j["connections"].is_array()) {
        for (const auto& c : j["connections"]) v.connections.push_back(c.get<int>());
    }
    if (j.contains("boundaryId") && j["boundaryId"].is_string()) {
        v.boundaryId = j["boundaryId"].get<std::string>();
    }
    if (j.contains("position") && j["position"].is_object()) {
        const auto& p = j["position"];
        if (p.contains("x") && p["x"].is_number()) {
            v.posX = p["x"].get<double>();
        }
        if (p.contains("y") && p["y"].is_number()) {
            v.posY = p["y"].get<double>();
        }
    }
    return v;
}

Json TemplateVertex::toJson() const {
    Json j;
    j["connections"] = connections;
    j["boundaryId"]  = boundaryId;
    j["position"]    = Json{ {"x", posX}, {"y", posY} };
    return j;
}

TemplateGraph TemplateGraph::import(const Json& j) {
    TemplateGraph g;
    if (j.contains("vertices") && j["vertices"].is_array()) {
        for (const auto& v : j["vertices"]) {
            g.vertices.push_back(TemplateVertex::import(v));
        }
    }
    if (j.contains("numEdges") && j["numEdges"].is_number_integer()) {
        g.numEdges = j["numEdges"].get<int>();
    }
    return g;
}

Json TemplateGraph::toJson() const {
    Json j;
    Json verts = Json::array();
    for (const auto& v : vertices) verts.push_back(v.toJson());
    j["vertices"] = verts;
    j["numEdges"] = numEdges;
    return j;
}

TemplateGraphSet TemplateGraphSet::import(const Json& j) {
    TemplateGraphSet s;
    if (j.contains("comment") && j["comment"].is_string()) {
        s.comment = j["comment"].get<std::string>();
    }
    if (j.contains("graphs") && j["graphs"].is_array()) {
        for (const auto& g : j["graphs"]) s.graphs.push_back(TemplateGraph::import(g));
    }
    return s;
}

Json TemplateGraphSet::toJson() const {
    Json j;
    j["comment"] = comment;
    Json gs = Json::array();
    for (const auto& g : graphs) gs.push_back(g.toJson());
    j["graphs"] = gs;
    return j;
}

std::vector<TemplateGraphSet> importTemplateGraphs(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("importTemplateGraphs: cannot open " + path);
    }
    std::stringstream buf;
    buf << in.rdbuf();
    Json root = Json::parse(buf.str());

    std::vector<TemplateGraphSet> out;
    if (root.is_array()) {
        // Standard format: array of entries.
        for (const auto& e : root) out.push_back(TemplateGraphSet::import(e));
    } else if (root.is_object()) {
        // Legacy single-entry: wrap it.
        out.push_back(TemplateGraphSet::import(root));
    } else {
        throw std::runtime_error(
            "importTemplateGraphs: root must be array or object in " + path);
    }
    return out;
}

bool exportTemplateGraphs(const std::string& path,
                          const std::vector<TemplateGraphSet>& sets) {
    Json root = Json::array();
    for (const auto& s : sets) root.push_back(s.toJson());
    std::ofstream out(path);
    if (!out) return false;
    out << root.dump(2);
    return true;
}
