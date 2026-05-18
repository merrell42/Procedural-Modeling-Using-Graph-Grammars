#include "pch.h"
#include "mesh_extraction/seed_grammar_writer.h"

#include "third_party/json.h"

#include <fstream>

namespace mesh_extraction {

namespace {

using Json = nlohmann::json;

Json vec3ToJson(const Vec3d& v) {
    Json j;
    j["x"] = v.x;
    j["y"] = v.y;
    j["z"] = v.z;
    return j;
}

// Build the seed Graph JSON matching the schema consumed by Graph::import.
// Produces both the topology arrays and the type-index arrays.
Json buildSeedGraphJson(const HalfEdgeMesh&   mesh,
                        const ExtractedTypes& types) {
    Json g;

    // Vertices: list of incident half-edge indices, slotted by the vertex
    // type's halfEdgeTypes order.
    Json vertices = Json::array();
    Json vertexTypesArr = Json::array();
    for (int v = 0; v < mesh.numVertices; ++v) {
        if (mesh.vertexHalves[v].empty()) continue;
        int vtIdx = types.vertexTypeOfVertex[v];
        int slots = (int)types.vertexTypes[vtIdx].halfEdgeTypes.size();
        std::vector<int> halves(slots, -1);
        for (int he : mesh.vertexHalves[v]) {
            int slot = types.halfEdgeSlotInVertex[he];
            if (slot >= 0 && slot < slots) halves[slot] = he;
        }
        Json vert;
        vert["halfEdges"] = halves;
        vertices.push_back(std::move(vert));

        Json vtRef;
        vtRef["kind"] = "v";
        vtRef["type"] = vtIdx;
        vertexTypesArr.push_back(std::move(vtRef));
    }

    // Edges: each edge has 2 "slots" of half-edges; pmugg schema stores them
    // as [[<halfA>], [<halfB>]].
    Json edges = Json::array();
    Json edgeTypesArr = Json::array();
    for (size_t ei = 0; ei < mesh.edges.size(); ++ei) {
        const auto& e = mesh.edges[ei];
        Json edge;
        Json slotA = Json::array(); slotA.push_back(e.halfA);
        Json slotB = Json::array();
        if (e.halfB >= 0) slotB.push_back(e.halfB);
        edge["halfEdges"] = { slotA, slotB };
        edges.push_back(std::move(edge));

        edgeTypesArr.push_back(types.edgeTypeOfEdge[ei]);
    }

    // Half-edges: forward / edgeIndex / vertexIndex / vertex / edge / prev /
    // next / face. Convention:
    //   forward    = (this half is the canonical halfA of its edge)
    //   edgeIndex  = 0 if forward (halfA slot), 1 otherwise
    //   vertexIndex = position of this halfEdge in the vertex's halfEdges array
    //                 (this is types.halfEdgeSlotInVertex[he])
    Json halfEdges = Json::array();
    for (size_t hi = 0; hi < mesh.halfEdges.size(); ++hi) {
        const auto& he = mesh.halfEdges[hi];
        const auto& edge = mesh.edges[he.edge];
        bool forward = (edge.halfA == (int)hi);

        Json j;
        j["forward"]     = forward;
        j["edgeIndex"]   = forward ? 0 : 1;
        j["vertexIndex"] = types.halfEdgeSlotInVertex[hi];
        j["vertex"]      = he.origin;
        j["edge"]        = he.edge;
        j["prev"]        = he.prev;
        j["next"]        = he.next;
        j["face"]        = he.face;
        halfEdges.push_back(std::move(j));
    }

    // Faces: each face's outerComponent is one half-edge on its boundary.
    Json faces = Json::array();
    Json faceTypesArr = Json::array();
    for (size_t fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto& f = mesh.faces[fi];
        Json face;
        face["outerComponent"] = f.halfEdges.empty() ? -1 : f.halfEdges.front();
        faces.push_back(std::move(face));

        faceTypesArr.push_back(types.faceTypeOfFace[fi]);
    }

    g["interior"]    = {
        {"vertices",  vertices},
        {"edges",     edges},
        {"halfEdges", halfEdges},
        {"faces",     faces},
    };
    g["vertexTypes"] = vertexTypesArr;
    g["edgeTypes"]   = edgeTypesArr;
    g["faceTypes"]   = faceTypesArr;
    // Closed shape — no boundary. Morphism arrays are empty.
    g["morphism"]    = {
        {"vertices", Json::array()},
        {"halfs",    Json::array()},
        {"faces",    Json::array()},
    };
    return g;
}

// Empty graph: one face with the "ambient" face type (faceType[0] by
// convention — we put the first volume's outward face there). This is the
// substrate the starter rule expands into.
Json buildEmptyGraphJson(const ExtractedTypes& /*types*/) {
    Json g;
    g["interior"] = {
        {"vertices",  Json::array()},
        {"edges",     Json::array()},
        {"halfEdges", Json::array()},
        {"faces",     Json::array({ Json{ {"outerComponent", -1} } })},
    };
    g["vertexTypes"] = Json::array();
    g["edgeTypes"]   = Json::array();
    g["faceTypes"]   = Json::array({ 0 });  // ambient
    g["morphism"]    = {
        {"vertices", Json::array()},
        {"halfs",    Json::array()},
        {"faces",    Json::array()},
    };
    return g;
}

}  // namespace

bool writeSeedGrammar(const HalfEdgeMesh&    mesh,
                      const ExtractedTypes&  types,
                      const std::string&     name,
                      const std::string&     outPath,
                      std::string*           error) {
    Json root;
    root["name"]     = name;
    // version 1 = current schema. Absent that tag, jsonMigration1 fires and
    // (per its legacy logic) discards everything not under a `solution` field.
    root["version"]  = 1;
    root["grounded"] = false;

    // ----- types: faceTypes, edgeTypes, vertexTypes ------------------------
    Json faceTypesJson = Json::array();
    for (const auto& ft : types.faceTypes) {
        Json j;
        j["material"] = ft.material;
        j["normal"]   = vec3ToJson(ft.normal);
        j["color"]    = nullptr;
        faceTypesJson.push_back(std::move(j));
    }

    Json edgeTypesJson = Json::array();
    for (const auto& et : types.edgeTypes) {
        Json j;
        j["dir"]          = vec3ToJson(et.dir);
        j["isRigid"]      = false;
        j["isRigidTiled"] = false;
        j["spliced"]      = false;
        Json fd = Json::array();
        for (const auto& f : et.faceData) {
            Json fj;
            fj["type"]    = f.faceType;
            fj["onRight"] = f.onRight;
            fd.push_back(std::move(fj));
        }
        j["faceData"] = fd;
        // Default edgeSettings matching the existing shipped grammars so the
        // mutator has sensible min/max lengths.
        j["edgeSettings"] = {
            {"Min Length", 0.1},
            {"Max Length", 4.0},
            {"Tile Length", 1.0},
            {"Rigid Tiled", false},
        };
        edgeTypesJson.push_back(std::move(j));
    }

    Json vertexTypesJson = Json::array();
    for (const auto& vt : types.vertexTypes) {
        Json j;
        j["spliced"] = false;
        Json het = Json::array();
        for (const auto& h : vt.halfEdgeTypes) {
            Json hj;
            hj["edge"]      = h.edgeType;
            hj["isAtStart"] = h.isAtStart;
            // dir is rederived from edge by VertexType::import if absent for
            // spliced types; for non-spliced it must be present. Use the edge
            // type's stored dir, negated if !isAtStart.
            const auto& d = types.edgeTypes[h.edgeType].dir;
            Vec3d hd = h.isAtStart ? d : Vec3d{-d.x, -d.y, -d.z};
            hj["dir"] = vec3ToJson(hd);
            het.push_back(std::move(hj));
        }
        j["halfEdgeTypes"] = het;
        vertexTypesJson.push_back(std::move(j));
    }

    Json typesJson;
    typesJson["dims"]        = 3;
    typesJson["faceTypes"]   = faceTypesJson;
    typesJson["edgeTypes"]   = edgeTypesJson;
    typesJson["vertexTypes"] = vertexTypesJson;
    root["types"] = typesJson;

    // ----- rules: empty `rules` / `groundRules`, one starter rule ---------
    root["rules"]       = Json::array();
    root["groundRules"] = Json::array();

    Json starterRule;
    starterRule["ground"] = false;
    Json emptyGraph = buildEmptyGraphJson(types);
    Json seedGraph  = buildSeedGraphJson(mesh, types);
    starterRule["n"] = Json::array({ emptyGraph, seedGraph });
    root["starterRules"] = Json::array({ starterRule });

    // ----- emptyGraph -----------------------------------------------------
    root["emptyGraph"] = buildEmptyGraphJson(types);

    // ----- write ----------------------------------------------------------
    std::ofstream out(outPath);
    if (!out) {
        if (error) *error = std::string("could not open for write: ") + outPath;
        return false;
    }
    out << root.dump(2);
    return true;
}

}  // namespace mesh_extraction
