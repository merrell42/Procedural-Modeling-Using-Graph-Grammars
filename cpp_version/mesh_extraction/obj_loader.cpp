// No PCH. tinyobjloader is a third-party single-header library; including
// the project PCH here would force tinyobj's implementation through it.
#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tinyobjloader/tiny_obj_loader.h"

#include "mesh_extraction/obj_loader.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace mesh_extraction {

// Read the entire file into a string. Returns empty string on failure (caller
// will then get tinyobjloader's own error on the subsequent parse).
static std::string slurpFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Scan an .obj text for `usemtl <name>` references and synthesize a minimal
// .mtl content (`newmtl <name>\n` per unique name). Reason: when an .obj
// references materials via usemtl but no .mtl ships alongside (common in the
// pmugg sample meshes), tinyobjloader drops the names — face.material_id
// becomes -1 and the label is lost. Pre-populating a synthetic .mtl lets
// every usemtl resolve to a real index whose name we can read back.
static std::string synthesizeMtlFromUsemtl(const std::string& objText) {
    std::unordered_set<std::string> seen;
    std::string out;
    size_t i = 0, n = objText.size();
    while (i < n) {
        // Find start of line content (skip leading whitespace).
        size_t lineStart = i;
        while (lineStart < n && (objText[lineStart] == ' ' || objText[lineStart] == '\t'))
            ++lineStart;
        if (lineStart + 6 < n && objText.compare(lineStart, 6, "usemtl") == 0 &&
            (objText[lineStart + 6] == ' ' || objText[lineStart + 6] == '\t')) {
            size_t name = lineStart + 7;
            while (name < n && (objText[name] == ' ' || objText[name] == '\t')) ++name;
            size_t end = name;
            while (end < n && objText[end] != '\n' && objText[end] != '\r') ++end;
            // Trim trailing whitespace.
            while (end > name && (objText[end - 1] == ' ' || objText[end - 1] == '\t')) --end;
            if (end > name) {
                std::string mat(objText, name, end - name);
                if (seen.insert(mat).second) {
                    out += "newmtl ";
                    out += mat;
                    out += '\n';
                }
            }
        }
        // Advance to next line.
        while (i < n && objText[i] != '\n') ++i;
        if (i < n) ++i;
    }
    return out;
}

bool loadObj(const char* path, ObjMesh& out, std::string* error) {
    out = {};

    tinyobj::ObjReaderConfig cfg;
    cfg.triangulate = false;  // preserve n-gons; the algorithm operates on polygon faces
    cfg.vertex_color = false;

    std::string objText = slurpFile(path);
    if (objText.empty()) {
        if (error) *error = std::string("could not open file: ") + path;
        return false;
    }
    std::string synthMtl = synthesizeMtlFromUsemtl(objText);

    // Strip any pre-existing `mtllib` directives. tinyobj's MaterialStreamReader
    // consumes its stream on first call; a second mtllib would error out. Then
    // prepend our own synthetic mtllib so the reader fires exactly once and
    // populates every name referenced by usemtl.
    {
        std::string filtered;
        filtered.reserve(objText.size() + synthMtl.size() + 32);
        filtered += "mtllib __synth__.mtl\n";
        size_t i = 0, n = objText.size();
        while (i < n) {
            size_t lineStart = i;
            size_t cur = i;
            while (cur < n && (objText[cur] == ' ' || objText[cur] == '\t')) ++cur;
            bool drop = (cur + 6 < n && objText.compare(cur, 6, "mtllib") == 0 &&
                         (objText[cur + 6] == ' ' || objText[cur + 6] == '\t'));
            while (i < n && objText[i] != '\n') ++i;
            if (i < n) ++i;
            if (!drop) filtered.append(objText, lineStart, i - lineStart);
        }
        objText.swap(filtered);
    }

    tinyobj::ObjReader reader;
    if (!reader.ParseFromString(objText, synthMtl, cfg)) {
        if (error) *error = reader.Error();
        return false;
    }
    if (error && !reader.Warning().empty()) *error = reader.Warning();

    const auto& attrib    = reader.GetAttrib();
    const auto& shapes    = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    out.vertices.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i + 2 < attrib.vertices.size(); i += 3) {
        out.vertices.push_back({attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]});
    }
    out.normals.reserve(attrib.normals.size() / 3);
    for (size_t i = 0; i + 2 < attrib.normals.size(); i += 3) {
        out.normals.push_back({attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]});
    }

    out.materialNames.reserve(materials.size());
    for (const auto& m : materials) out.materialNames.push_back(m.name);

    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;
        size_t corner = 0;
        for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
            int n = mesh.num_face_vertices[f];
            ObjMesh::Face face;
            face.corners.reserve(n);
            for (int k = 0; k < n; ++k) {
                const tinyobj::index_t& idx = mesh.indices[corner + k];
                face.corners.push_back({idx.vertex_index, idx.normal_index, idx.texcoord_index});
            }
            face.materialId = (f < mesh.material_ids.size()) ? mesh.material_ids[f] : -1;
            face.groupName  = shape.name;
            out.faces.push_back(std::move(face));
            corner += n;
        }
    }
    return true;
}

}  // namespace mesh_extraction
