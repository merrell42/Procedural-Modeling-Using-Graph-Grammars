#include "pch.h"
#include "TemplateGraph.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

TemplateEdge TemplateEdge::import(const Json& j) {
    TemplateEdge e;
    if (j.contains("start") && j["start"].is_number_integer()) {
        e.start = j["start"].get<int>();
    }
    if (j.contains("end") && j["end"].is_number_integer()) {
        e.end = j["end"].get<int>();
    }
    return e;
}

Json TemplateEdge::toJson() const {
    return Json{ {"start", start}, {"end", end} };
}

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
    if (j.contains("edges") && j["edges"].is_array()) {
        for (const auto& e : j["edges"]) {
            g.edges.push_back(TemplateEdge::import(e));
        }
    }
    return g;
}

Json TemplateGraph::toJson() const {
    Json j;
    Json verts = Json::array();
    for (const auto& v : vertices) verts.push_back(v.toJson());
    j["vertices"] = verts;
    Json edgeArr = Json::array();
    for (const auto& e : edges) edgeArr.push_back(e.toJson());
    j["edges"] = edgeArr;
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
        for (const auto& e : root) out.push_back(TemplateGraphSet::import(e));
    } else {
        throw std::runtime_error(
            "importTemplateGraphs: root must be array in " + path);
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
