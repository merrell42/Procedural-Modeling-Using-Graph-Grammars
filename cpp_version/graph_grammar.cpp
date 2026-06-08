#include "pch.h"
#include "graph_grammar.h"
#include "graph/graph.h"
#include "graph/graph_vertex.h"
#include "graph/graph_edge.h"
#include "graph/graph_half_edge.h"
#include "graph/graph_face.h"
#include "graph/edge_settings.h"
#include "grammar_rules/production_rule.h"
#include "primitives/edge_type.h"
#include "primitives/face_type.h"
#include "primitives/vertex_type.h"
#include "settings.h"
#include "util/util.h"
#include "util/binary_stream.h"
#include "json versioning/json_version_manager.h"
#include "json versioning/json_migrations.h"
#include <algorithm>
#include <unordered_map>

GraphGrammar::GraphGrammar() {
    emptyGraph = nullptr;
    primitives = nullptr;
    grounded = false;
}

GraphGrammar::GraphGrammar(Primitives* primitives_) {
    primitives = primitives_;
    // emptyGraph = Graph::createEmpty(primitives_);
    emptyGraph = new Graph();
    grounded = false;
}

GraphGrammar::~GraphGrammar() {
    delete emptyGraph;
    for (auto rule : starterRules) {
        delete rule;
    }
    for (auto rule : rules) {
        delete rule;
    }
    for (auto rule : groundRules) {
        delete rule;
    }
    delete primitives;
}

bool GraphGrammar::isGrounded() const {
    return grounded && groundRules.size() > 0;
}

void GraphGrammar::addRule(ProductionRule* rule) {
    rules.push_back(rule);
}

void GraphGrammar::addStarterRule(ProductionRule* rule) {
    starterRules.push_back(rule);
}

// Randomly pick one of the normal production rules.
Production GraphGrammar::getProduction() {
    ProductionRule* rule = rules.empty() ? nullptr : Util::pick<ProductionRule*>(rules);
    if (!rule) {
        return { nullptr, nullptr, nullptr, false };
    }
    auto startGraphs = rule->getStartGraphs();
    auto endGraphs = rule->getEndGraphs();
    int n = (int)startGraphs.size();

    // From the rule pick one start graph and one end graph that is different.
    int start = Util::randomInt(n);
    int end = Util::randomInt(n - 1);
    if (end >= start) {
        end++;
    }
    return { startGraphs[start], endGraphs[end], nullptr, false};
}

// Find a production that starts with an empty graph and adds a new graph.
// If useGround is true we take it from the ground rules.
Production GraphGrammar::getStarterProduction(bool useGround) {
    bool isGrounded = (grounded && useGround) || starterRules.empty();
    auto& rules = isGrounded ? groundRules : starterRules;

    auto* rule = Util::pick(rules);
    if (rule) {
        auto startGraphs = rule->getStartGraphs();
        auto endGraphs = rule->getEndGraphs();
        int n = (int)endGraphs.size();
        auto* startGraph = startGraphs[0];
        auto* endGraph = endGraphs[Util::randomInt(n - 1) + 1];
        return { startGraph, endGraph, nullptr, rule->isGround() };
    }
    return { nullptr, nullptr, nullptr, false };
}

// Find the opposite of getStarterProduction that is a production that starts with
// graph and replaces it with the empty graph.
Production GraphGrammar::getRemovalProduction() {
    if (starterRules.empty()) {
        return {nullptr, nullptr, nullptr, false};
    }
    auto* rule = Util::pick(starterRules);
    if (!rule) {
        return { nullptr, nullptr, nullptr, false };
    }
    auto startGraphs = rule->getStartGraphs();
    int n = (int)startGraphs.size();
    auto* endGraph = emptyGraph;
    auto* startGraph = startGraphs[Util::randomInt(n - 1) + 1];
    return {startGraph, endGraph, nullptr, false};
    
}

GraphGrammar* GraphGrammar::import(const Json& json) {
    auto* grammar = new GraphGrammar();
    grammar->primitives = Primitives::import(json["types"]);
    
    auto importRule = [&](const Json& transJson) {
        return ProductionRule::import(transJson, grammar->primitives);
    };
    
    for (const auto& transJson : json["rules"]) {
        grammar->rules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["starterRules"]) {
        grammar->starterRules.push_back(importRule(transJson));
    }
    for (const auto& transJson : json["groundRules"]) {
        grammar->groundRules.push_back(importRule(transJson));
    }
    
    grammar->grounded = json["grounded"];
    grammar->emptyGraph = Graph::import(json["emptyGraph"], grammar->primitives);

    return grammar;
}

Json GraphGrammar::exportJson() const {
    Json json;
    registerJsonMigrations();
    json["version"] = JsonVersionManager::getLatestVersion();
    
    Json rulesJson = Json::array();
    for (const auto* rule : rules) {
        rulesJson.push_back(rule->exportJson(primitives));
    }
    json["rules"] = rulesJson;

    Json starterRulesJson = Json::array();
    for (const auto* rule : starterRules) {
        starterRulesJson.push_back(rule->exportJson(primitives));
    }
    json["starterRules"] = starterRulesJson;

    Json groundRulesJson = Json::array();
    for (const auto* rule : groundRules) {
        groundRulesJson.push_back(rule->exportJson(primitives));
    }
    json["groundRules"] = groundRulesJson;

    json["types"] = primitives->exportJson();
    json["grounded"] = grounded;
    json["emptyGraph"] = emptyGraph->exportJson(primitives);
    return json;
}

// ---------------------------------------------------------------------------
// Binary serialization
// Layout: primitives | grounded | starterRules | rules | groundRules | emptyGraph.
// Cross-references (pointers) are stored as indices into the parent collection.
// ---------------------------------------------------------------------------

namespace {

struct GraphIndexMaps {
    std::unordered_map<GraphVertex*,   int32_t> v;
    std::unordered_map<GraphEdge*,     int32_t> e;
    std::unordered_map<GraphHalfEdge*, int32_t> he;
    std::unordered_map<GraphFace*,     int32_t> f;
};

struct PrimIndexMaps {
    std::unordered_map<VertexType*, int32_t> vt;
    std::unordered_map<EdgeType*,   int32_t> et;
    std::unordered_map<FaceType*,   int32_t> ft;
};

GraphIndexMaps buildIndexMaps(const Graph* g) {
    GraphIndexMaps m;
    const auto& vv = g->getVertices();
    const auto& ee = g->getEdges();
    const auto& hh = g->getHalfEdges();
    const auto& ff = g->getFaces();
    for (int32_t i = 0; i < (int32_t)vv.size();  i++) m.v[vv[i]]  = i;
    for (int32_t i = 0; i < (int32_t)ee.size();  i++) m.e[ee[i]]  = i;
    for (int32_t i = 0; i < (int32_t)hh.size();  i++) m.he[hh[i]] = i;
    for (int32_t i = 0; i < (int32_t)ff.size();  i++) m.f[ff[i]]  = i;
    return m;
}

PrimIndexMaps buildPrimMaps(const Primitives* p) {
    PrimIndexMaps m;
    for (int32_t i = 0; i < (int32_t)p->vertexTypes.size(); i++) m.vt[p->vertexTypes[i]] = i;
    for (int32_t i = 0; i < (int32_t)p->edgeTypes.size();   i++) m.et[p->edgeTypes[i]]   = i;
    for (int32_t i = 0; i < (int32_t)p->faceTypes.size();   i++) m.ft[p->faceTypes[i]]   = i;
    return m;
}

int32_t heIdx(const GraphIndexMaps& m, GraphHalfEdge* h) {
    if (!h) return -1;
    auto it = m.he.find(h);
    return it != m.he.end() ? it->second : -1;
}
int32_t vIdx(const GraphIndexMaps& m, GraphVertex* v) {
    if (!v) return -1;
    auto it = m.v.find(v);
    return it != m.v.end() ? it->second : -1;
}
int32_t fIdx(const GraphIndexMaps& m, GraphFace* f) {
    if (!f) return -1;
    auto it = m.f.find(f);
    return it != m.f.end() ? it->second : -1;
}

void serializeEdgeSettings(std::ostream& out, const EdgeSettings* es) {
    const auto& dbl  = es->getDoubleProperties();
    const auto& bls  = es->getBoolProperties();
    const auto& strs = es->getStringProperties();
    bsWrite<int32_t>(out, (int32_t)dbl.size());
    for (const auto& [k, v] : dbl)  { bsWriteStr(out, k); bsWrite<double>(out, v); }
    bsWrite<int32_t>(out, (int32_t)bls.size());
    for (const auto& [k, v] : bls)  { bsWriteStr(out, k); bsWrite<uint8_t>(out, v ? 1 : 0); }
    bsWrite<int32_t>(out, (int32_t)strs.size());
    for (const auto& [k, v] : strs) { bsWriteStr(out, k); bsWriteStr(out, v); }
}

void serializePrimitives(std::ostream& out, const Primitives* p) {
    bsWrite<int32_t>(out, p->dims);

    bsWrite<int32_t>(out, (int32_t)p->faceTypes.size());
    for (const auto* ft : p->faceTypes) {
        bsWriteStr(out,  ft->getMaterial());
        bsWriteVec3(out, ft->getNormal());
        bsWriteVec3(out, ft->getColor());
    }

    bsWrite<int32_t>(out, (int32_t)p->edgeTypes.size());
    for (const auto* et : p->edgeTypes) {
        bsWriteVec3(out, et->getDir());
        bsWrite<uint8_t>(out, et->getIsRigid() ? 1 : 0);
        bsWrite<uint8_t>(out, et->getSpliced() ? 1 : 0);
        bsWriteStr(out, et->getRuleGeneratorId());

        const auto& fd = et->getFaceData();
        bsWrite<int32_t>(out, (int32_t)fd.size());
        for (const auto& d : fd) {
            int32_t ftIdx = -1;
            for (int32_t k = 0; k < (int32_t)p->faceTypes.size(); k++) {
                if (p->faceTypes[k] == d.type) { ftIdx = k; break; }
            }
            bsWrite<int32_t>(out, ftIdx);
            bsWrite<uint8_t>(out, d.onRight ? 1 : 0);
        }

        const EdgeSettings* es = et->getEdgeSettings();
        bsWrite<uint8_t>(out, es ? 1 : 0);
        if (es) serializeEdgeSettings(out, es);
    }

    bsWrite<int32_t>(out, (int32_t)p->vertexTypes.size());
    for (const auto* vt : p->vertexTypes) {
        bsWrite<uint8_t>(out, vt->getSpliced() ? 1 : 0);
        bsWrite<int32_t>(out, vt->getRuleGeneratorId());
        const auto& hets = vt->getHalfEdgeTypes();
        bsWrite<int32_t>(out, (int32_t)hets.size());
        for (const auto& het : hets) {
            int32_t etIdx = -1;
            for (int32_t k = 0; k < (int32_t)p->edgeTypes.size(); k++) {
                if (p->edgeTypes[k] == het.edge) { etIdx = k; break; }
            }
            bsWrite<int32_t>(out, etIdx);
            bsWrite<uint8_t>(out, het.isAtStart ? 1 : 0);
            bsWriteVec3(out, het.dir);
        }
    }
}

Primitives* deserializePrimitives(std::istream& in) {
    int32_t dims = bsRead<int32_t>(in);
    auto* p = new Primitives(dims);

    int32_t ftCount = bsRead<int32_t>(in);
    p->faceTypes.reserve(ftCount);
    for (int32_t i = 0; i < ftCount; i++)
        p->faceTypes.push_back(FaceType::binaryDeserialize(in));

    int32_t etCount = bsRead<int32_t>(in);
    p->edgeTypes.reserve(etCount);
    for (int32_t i = 0; i < etCount; i++)
        p->edgeTypes.push_back(EdgeType::binaryDeserialize(in, p));

    int32_t vtCount = bsRead<int32_t>(in);
    p->vertexTypes.reserve(vtCount);
    for (int32_t i = 0; i < vtCount; i++)
        p->vertexTypes.push_back(VertexType::binaryDeserialize(in, p));

    return p;
}

void serializeGraph(std::ostream& out, const Graph* g, const PrimIndexMaps& pm) {
    const auto& vv = g->getVertices();
    const auto& ee = g->getEdges();
    const auto& hh = g->getHalfEdges();
    const auto& ff = g->getFaces();

    bsWrite<int32_t>(out, (int32_t)vv.size());
    bsWrite<int32_t>(out, (int32_t)ee.size());
    bsWrite<int32_t>(out, (int32_t)hh.size());
    bsWrite<int32_t>(out, (int32_t)ff.size());

    GraphIndexMaps gm = buildIndexMaps(g);

    for (auto* v : vv) {
        const auto& slots = v->getHalfEdges();
        bsWrite<int32_t>(out, (int32_t)slots.size());
        for (auto* h : slots) bsWrite<int32_t>(out, heIdx(gm, h));

        auto it = pm.vt.find(v->getType());
        bsWrite<int32_t>(out, it != pm.vt.end() ? it->second : -1); // -1 = edgeVertex placeholder
    }

    for (const auto* e : ee) {
        const auto& halfs = e->getHalfEdges();
        bsWrite<int32_t>(out, (int32_t)halfs.size());
        for (const auto& inner : halfs) {
            bsWrite<int32_t>(out, (int32_t)inner.size());
            for (auto* h : inner) bsWrite<int32_t>(out, heIdx(gm, h));
        }
        bsWrite<int32_t>(out, pm.et.at(e->getType()));
    }

    for (auto* h : hh) {
        bsWrite<uint8_t>(out, h->getForward() ? 1 : 0);
        bsWrite<int32_t>(out, h->getVertexIndex());
        bsWrite<int32_t>(out, h->getEdgeIndex());
        bsWrite<int32_t>(out, vIdx(gm, h->getVertex()));
        auto* edge = h->getEdge();
        int32_t ei = -1;
        if (edge) { auto it = gm.e.find(edge); if (it != gm.e.end()) ei = it->second; }
        bsWrite<int32_t>(out, ei);
        bsWrite<int32_t>(out, heIdx(gm, h->getPrev()));
        bsWrite<int32_t>(out, heIdx(gm, h->getNext()));
        bsWrite<int32_t>(out, fIdx(gm, h->getFace()));
    }

    for (const auto* f : ff) {
        bsWrite<int32_t>(out, heIdx(gm, f->getOuterComponent()));
        bsWrite<int32_t>(out, pm.ft.at(f->getType()));
    }

    const auto& bv = g->getBVertices();
    bsWrite<int32_t>(out, (int32_t)bv.size());
    for (auto* v : bv) bsWrite<int32_t>(out, vIdx(gm, v));

    const auto& bh = g->getBHalfEdges();
    bsWrite<int32_t>(out, (int32_t)bh.size());
    for (auto* h : bh) bsWrite<int32_t>(out, heIdx(gm, h));

    const auto& bf = g->getBFaces();
    bsWrite<int32_t>(out, (int32_t)bf.size());
    for (auto* f : bf) bsWrite<int32_t>(out, fIdx(gm, f));
}

void serializeProductionRule(std::ostream& out, const ProductionRule* rule,
                              const PrimIndexMaps& pm) {
    bsWrite<uint8_t>(out, rule->isGround() ? 1 : 0);
    const auto& starts = rule->getStartGraphs();
    const auto& ends   = rule->getEndGraphs();
    bsWrite<int32_t>(out, (int32_t)starts.size());
    for (int32_t i = 0; i < (int32_t)starts.size(); i++) {
        serializeGraph(out, starts[i], pm);
        serializeGraph(out, ends[i],   pm);
    }
}

} // namespace

void GraphGrammar::serialize(std::ostream& out) const {
    serializePrimitives(out, primitives);

    PrimIndexMaps pm = buildPrimMaps(primitives);

    bsWrite<uint8_t>(out, grounded ? 1 : 0);

    auto writeRules = [&](const std::vector<ProductionRule*>& rs) {
        bsWrite<int32_t>(out, (int32_t)rs.size());
        for (const auto* r : rs) serializeProductionRule(out, r, pm);
    };
    writeRules(starterRules);
    writeRules(rules);
    writeRules(groundRules);

    serializeGraph(out, emptyGraph, pm);
}

GraphGrammar* GraphGrammar::deserialize(std::istream& in) {
    auto* grammar        = new GraphGrammar();
    grammar->primitives  = deserializePrimitives(in);
    grammar->grounded    = bsRead<uint8_t>(in) != 0;

    auto readRules = [&](std::vector<ProductionRule*>& rs) {
        int32_t n = bsRead<int32_t>(in);
        rs.reserve(n);
        for (int32_t i = 0; i < n; i++)
            rs.push_back(ProductionRule::binaryDeserialize(in, grammar->primitives));
    };
    readRules(grammar->starterRules);
    readRules(grammar->rules);
    readRules(grammar->groundRules);

    grammar->emptyGraph = Graph::binaryDeserialize(in, grammar->primitives);
    return grammar;
}
