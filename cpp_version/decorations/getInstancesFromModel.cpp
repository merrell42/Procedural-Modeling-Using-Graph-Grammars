#include "pch.h"
#include "getInstancesFromModel.h"
#include "decoration_output.h"
#include "../graph_drawing/model.h"
#include "../graph_drawing/graph_drawing.h"
#include "../graph_drawing/vertex.h"
#include "../graph_drawing/edge.h"
#include "../graph_drawing/face.h"
#include "../graph_drawing/half_edge.h"
#include "../primitives/vertex_type.h"
#include "../primitives/edge_type.h"
#include "../primitives/face_type.h"
#include "../decorations/vertex_decoration.h"
#include "../decorations/edge_decoration.h"
#include "../decorations/face_decoration.h"
#include "../geometry/matrix4.h"
#include "../util/diagnostics.h"
#include "../util/util.h"
#include <cstring>
#include <map>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

char* copyCString(const char* value) {
    size_t length = strlen(value) + 1;
    char* copy = (char*)malloc(length);
    memcpy(copy, value, length);
    return copy;
}

struct CollectedDecorations {
    vector<Instance> instances;
    vector<Extrusion> extrusions;
    Model* model = nullptr;
    bool dirty = true;
};

CollectedDecorations collected;

void appendOutput(const DecorationOutput& output) {
    Util::append(collected.instances, output.instances);
    Util::append(collected.extrusions, output.extrusions);
}

void collectDecorations(Model* model) {
    // Skip if nothing has changed.
    if (!collected.dirty && collected.model == model) {
        return;
    }
    collected.dirty = true;
    collected.instances.clear();
    collected.extrusions.clear();
    collected.model = model;

    GraphDrawing* drawing = model->getCurrent();

    for (auto& [id, vertex] : drawing->getVertexMap()) {
        VertexDecoration* decoration = vertex->getType()->getDecoration();
        if (decoration) {
            const Vec3 position = vertex->getPosition();
            appendOutput(decoration->getOutput(Matrix4::translation(position)));
        }
    }

    for (auto& [id, edge] : drawing->getEdgeMap()) {
        EdgeDecoration* decoration = edge->getEdgeType()->getDecoration();
        if (decoration) {
            const vector<HalfEdge*> halfEdges = edge->getHalfEdges();
            appendOutput(decoration->getOutput(
                halfEdges[0]->getPosition(),
                halfEdges[1]->getPosition()
            ));
        }
    }

    for (auto& [id, face] : drawing->getFaceMap()) {
        FaceDecoration* decoration = face->getFaceType()->getDecoration();
        if (decoration) {
            appendOutput(decoration->getOutput(*face));
        }
    }
    collected.dirty = false;
}

} // namespace

void markOutputDirty() {
    collected.dirty = true;
    collected.instances.clear();
    collected.extrusions.clear();
    collected.model = nullptr;
}

InstanceList getInstancesFromModel(Model* model) {
    InstanceList emptyList{};
    emptyList.instances = nullptr;
    emptyList.count = 0;

    try {
        collectDecorations(model);
        const vector<Instance>& instances = collected.instances;
        InstanceList instanceList{};
        instanceList.count = (int)instances.size();
        if (instanceList.count == 0) {
            instanceList.instances = nullptr;
            return instanceList;
        }

        instanceList.instances = (Instance*)malloc(instanceList.count * sizeof(Instance));
        for (int i = 0; i < instanceList.count; i++) {
            instanceList.instances[i].assetId = copyCString(instances[i].assetId);
            instanceList.instances[i].transform = instances[i].transform;
        }
        return instanceList;
    } catch (const exception& e) {
        Diagnostics::setWarning(string("Warning: ") + e.what());
        return emptyList;
    }
}

void appendExtrusions(MeshCpp& mesh, Model* model) {
    try {
        collectDecorations(model);
    } catch (const exception& e) {
        Diagnostics::setWarning(string("Warning: ") + e.what());
        return;
    }

    struct ColorGroup {
        Color color;
        vector<Vec3> positions;
        vector<Vec3> normals;
        vector<int> triangles;
    };
    map<Color, int> groupIndex;
    vector<ColorGroup> groups;
    for (const Extrusion& extrusion : collected.extrusions) {
        // Find or create the group for this color.
        const Color& key = extrusion.color;
        auto found = groupIndex.find(key);
        if (found == groupIndex.end()) {
            found = groupIndex.insert({ key, (int)groups.size() }).first;
            groups.push_back(ColorGroup{ key, {}, {}, {} });
        }

        // Add the extrusion to the group.
        ColorGroup& group = groups[found->second];
        int offset = (int)group.positions.size();
        Util::append(group.positions, extrusion.positions);
        Util::append(group.normals, extrusion.normals);
        for (int index : extrusion.triangles) {
            group.triangles.push_back(index + offset);
        }
    }
    if (groups.empty()) {
        return;
    }

    int total = mesh.numSubmeshes + (int)groups.size();
    SubmeshCpp* resized = (SubmeshCpp*)realloc(mesh.submeshes, total * sizeof(SubmeshCpp));
    if (!resized) {
        return;
    }
    mesh.submeshes = resized;
    for (int i = 0; i < (int)groups.size(); i++) {
        mesh.submeshes[mesh.numSubmeshes + i] = createSubmesh(
            groups[i].positions,
            groups[i].normals,
            groups[i].triangles,
            {},
            groups[i].color.red,
            groups[i].color.green,
            groups[i].color.blue
        );
    }
    mesh.numSubmeshes = total;
}
