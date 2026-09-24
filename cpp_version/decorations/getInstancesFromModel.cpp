#include "pch.h"
#include "getInstancesFromModel.h"
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
#include <cstring>
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

} // namespace

InstanceList getInstancesFromModel(Model* model) {
    InstanceList emptyList{};
    emptyList.instances = nullptr;
    emptyList.count = 0;

    try {
        if (!model) {
            return emptyList;
        }

        vector<Instance> collected;

        auto appendInstances = [&](const vector<Instance>& instances) {
            collected.insert(collected.end(), instances.begin(), instances.end());
        };

        GraphDrawing* drawing = model->getCurrent();

        for (auto& [id, vertex] : drawing->getVertexMap()) {
            VertexType* type = vertex->getType();
            VertexDecoration* decoration = type ? type->getDecoration() : nullptr;
            if (!decoration) {
                continue;
            }
            const Vec3 position = vertex->getPosition();
            Matrix4 transform = Matrix4::translation(position);
            appendInstances(decoration->getInstances(transform));
        }

        for (auto& [id, edge] : drawing->getEdgeMap()) {
            EdgeType* type = edge->getEdgeType();
            EdgeDecoration* decoration = type ? type->getDecoration() : nullptr;
            if (!decoration) {
                continue;
            }
            const vector<HalfEdge*> halfEdges = edge->getHalfEdges();
            if (halfEdges.size() < 2 || !halfEdges[0] || !halfEdges[1]) {
                continue;
            }
            appendInstances(decoration->getInstances(
                halfEdges[0]->getPosition(),
                halfEdges[1]->getPosition()
            ));
        }

        for (auto& [id, face] : drawing->getFaceMap()) {
            FaceType* type = face->getFaceType();
            FaceDecoration* decoration = type ? type->getDecoration() : nullptr;
            if (!decoration) {
                continue;
            }
            appendInstances(decoration->getInstances(*face));
        }

        InstanceList instanceList{};
        instanceList.count = (int)collected.size();
        if (instanceList.count == 0) {
            instanceList.instances = nullptr;
            return instanceList;
        }

        instanceList.instances = (Instance*)malloc(instanceList.count * sizeof(Instance));
        for (int i = 0; i < instanceList.count; i++) {
            instanceList.instances[i].assetId = copyCString(collected[i].assetId);
            instanceList.instances[i].transform = collected[i].transform;
        }

        return instanceList;
    } catch (const exception& e) {
        Diagnostics::setWarning(string("Warning: ") + e.what());
        return emptyList;
    }
}
