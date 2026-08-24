#pragma once
// The Graph Template Data Structure.

#include <string>
#include <vector>
#include "json.h"

using Json = nlohmann::json;

// One edge in a graph template.
struct TemplateEdge {
    int start = 0;
    int end = 0;
    bool spliced = false;

    static TemplateEdge import(const Json& j);
    Json toJson() const;
};

// One vertex in a graph template.
struct TemplateVertex {
    // Edge indices incident to this vertex, ordered CCW around the vertex
    // (the editor records this order on edge insertion). The matcher reads
    // `connections` to build its edge → vertex adjacency table.
    std::vector<int> connections;

    // Empty string = interior vertex. A non-empty value identifies a
    // boundary vertex shared with the paired template; vertices in the two
    // templates carrying the same `boundaryId` are linked when the matcher
    // emits rule output.
    std::string boundaryId;

    // Editor canvas position. The matcher does not use this; it round-trips
    // so a template can be saved without losing its visual layout.
    double posX = 0.0;
    double posY = 0.0;
    bool spliced = false;

    static TemplateVertex import(const Json& j);
    Json toJson() const;
};

// One graph in a template set (left or right side of a production rule).
struct TemplateGraph {
    std::vector<TemplateVertex> vertices;
    std::vector<TemplateEdge> edges;

    static TemplateGraph import(const Json& j);
    Json toJson() const;
};

// One entry in the library file: a comment + the graphs that form a rule.
// Usually there are just two graphs per entry (start side, end side).
// We store an N-vector for forward-compatibility.
struct TemplateGraphSet {
    std::string comment;
    std::vector<TemplateGraph> graphs;

    static TemplateGraphSet import(const Json& j);
    Json toJson() const;
};

// A template library file: every graph-template entry plus a file-level
// flag for whether a ground rule should be generated.
struct TemplateLibrary {
    bool includeGround = false;
    std::vector<TemplateGraphSet> sets;
};

// Reads the library JSON at `path`. Accepts a bare array of entries
// (includeGround defaults to false) or an object with `includeGround`
// and `templates`. Throws on I/O or schema errors.
TemplateLibrary importTemplateGraphs(const std::string& path);

// Symmetric writer. Serializes as `{ includeGround, templates }`.
// Returns false on I/O failure.
bool exportTemplateGraphs(const std::string& path, const TemplateLibrary& library);
