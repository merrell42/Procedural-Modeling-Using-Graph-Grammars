// No PCH. tinyobjloader is a third-party single-header library; including
// the project PCH here would force tinyobj's implementation through it.
#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tinyobjloader/tiny_obj_loader.h"

#include "mesh_extraction/obj_loader.h"

#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

static std::string directoryPrefix(const char* path) {
    std::string s(path ? path : "");
    size_t slash = s.find_last_of("/\\");
    if (slash == std::string::npos) return {};
    return s.substr(0, slash + 1);
}

// Rest of the line after a keyword such as `mtllib` / `newmtl` / `Kd`.
static std::string restOfLine(const std::string& text, size_t keywordEnd, size_t lineEnd) {
    size_t i = keywordEnd;
    while (i < lineEnd && (text[i] == ' ' || text[i] == '\t')) ++i;
    size_t end = lineEnd;
    while (end > i && (text[end - 1] == ' ' || text[end - 1] == '\t')) --end;
    if (end <= i) return {};
    return std::string(text, i, end - i);
}

static void forEachContentLine(
    const std::string& text,
    const std::function<void(size_t start, size_t end)>& fn
) {
    size_t i = 0, n = text.size();
    while (i < n) {
        size_t lineStart = i;
        while (lineStart < n && (text[lineStart] == ' ' || text[lineStart] == '\t'))
            ++lineStart;
        size_t lineEnd = lineStart;
        while (lineEnd < n && text[lineEnd] != '\n' && text[lineEnd] != '\r')
            ++lineEnd;
        if (lineStart < lineEnd && text[lineStart] != '#') {
            fn(lineStart, lineEnd);
        }
        i = lineEnd;
        if (i < n && text[i] == '\r') ++i;
        if (i < n && text[i] == '\n') ++i;
    }
}

// Wavefront MTL stores diffuse color as `Kd`. We keep the synthetic-mtl path
// for name resolution, and read Kd from the real file so extraction can write
// it into face types.
static std::unordered_map<std::string, ObjMesh::Color>
loadDiffuseColors(const char* objPath, const std::string& objText) {
    std::vector<std::string> libs;
    forEachContentLine(objText, [&](size_t start, size_t end) {
        if (end - start >= 6 && objText.compare(start, 6, "mtllib") == 0 &&
            (start + 6 == end || objText[start + 6] == ' ' || objText[start + 6] == '\t')) {
            std::string name = restOfLine(objText, start + 6, end);
            if (!name.empty()) libs.push_back(std::move(name));
        }
    });

    const std::string prefix = directoryPrefix(objPath);
    std::unordered_map<std::string, ObjMesh::Color> colors;
    for (const auto& lib : libs) {
        std::string mtlText = slurpFile((prefix + lib).c_str());
        if (mtlText.empty()) continue;
        std::string current;
        forEachContentLine(mtlText, [&](size_t start, size_t end) {
            if (end - start >= 6 && mtlText.compare(start, 6, "newmtl") == 0 &&
                (start + 6 == end || mtlText[start + 6] == ' ' || mtlText[start + 6] == '\t')) {
                current = restOfLine(mtlText, start + 6, end);
            } else if (!current.empty() && end - start >= 2 &&
                       mtlText.compare(start, 2, "Kd") == 0 &&
                       (start + 2 == end || mtlText[start + 2] == ' ' || mtlText[start + 2] == '\t')) {
                std::istringstream iss(restOfLine(mtlText, start + 2, end));
                double r = 1.0, g = 1.0, b = 1.0;
                if (iss >> r >> g >> b) {
                    colors[current] = {r, g, b};
                }
            }
        });
    }
    return colors;
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
    auto diffuseColors = loadDiffuseColors(path, objText);
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
    out.materialColors.reserve(materials.size());
    const ObjMesh::Color white{1.0, 1.0, 1.0};
    for (const auto& m : materials) {
        out.materialNames.push_back(m.name);
        auto it = diffuseColors.find(m.name);
        out.materialColors.push_back(it != diffuseColors.end() ? it->second : white);
    }

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
