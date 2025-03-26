#include "transistor_path.h"
#include "../util/util.h"
#include "../graph_drawing/endpoint.h"
#include <iostream>

namespace ms {

// Initialize static counter
int TransistorPath::count = 0;

TransistorPath* TransistorPath::createNet(const std::vector<Endpoint*>& endpoints,
                                        const std::vector<Line*>& edges,
                                        std::vector<Line*>* lines) {
    std::vector<IndexInfo> indices;
    for (auto* endpoint : endpoints) {
        auto it = std::find_if(edges.begin(), edges.end(),
            [endpoint](Line* edge) {
                return edge == endpoint->getLine();
            });
        if (it == edges.end()) {
            std::cout << "endpoint is not found in edges." << std::endl;
            return nullptr;
        }
        int index = std::distance(edges.begin(), it);
        indices.push_back({index, !endpoint->getIsAtStart()});
    }
    return new TransistorPath(indices, lines);
}

//TransistorPath* TransistorPath::create(const std::vector<Endpoint*>& endpoints,
//                                     const std::vector<Line*>& edges,
//                                     const std::vector<Line*>& lines) {
//    std::vector<IndexInfo> indices;
//    for (auto* endpoint : endpoints) {
//        auto* edge = endpoint->getEdge();
//        auto it = std::find(edges.begin(), edges.end(), edge);
//        if (it == edges.end()) {
//            Alert::show("endpoint is not found in edges.");
//            return nullptr;
//        }
//        int index = std::distance(edges.begin(), it);
//        indices.push_back({index, endpoint->getEdgeIndex() == 0});
//    }
//    return new TransistorPath(indices, lines);
//}

TransistorPath::TransistorPath(const std::vector<IndexInfo>& indices,
                             std::vector<Line*>* lines)
    : indices(indices)
    , lines(lines)
    , extendable{true, true}
    , id(count++) {
}

void TransistorPath::setEndpoints(const std::vector<Endpoint*>& endpoints) {
    this->endpoints = endpoints;
}

int TransistorPath::extendableness() const {
    return extendable[0] + extendable[1];
}

Vertex* TransistorPath::randomNextVertex() {
    std::vector<double> probabilities;
    for (bool e : extendable) {
        probabilities.push_back(e ? 1.0 : 0.0);
    }
    int index = Util::randomDistribution(probabilities);
    return endpoints[index]->getVertex();
}

Vertex* TransistorPath::rigidNextVertex() {
    for (int i = 0; i < 2; i++) {
        if (extendable[i] && indices.size() >= 2) {
            std::vector<int> iIndices;
            if (i == 0) {
                iIndices = {0, 1};
            } else {
                iIndices = {static_cast<int>(indices.size() - 2),
                           static_cast<int>(indices.size() - 1)};
            }
            // Two consecutive indices must be rigid
            bool rigid = true;
            for (int j = 0; j < 2; j++) {
                auto* line = lineFromIndex(iIndices[j]);
                rigid = rigid && !line->getEdgeType()->extendable();
            }
            if (rigid) {
                return endpoints[i]->getVertex();
            }
        }
    }
    return nullptr;
}

Line* TransistorPath::lineFromIndex(int index) {
    return (*lines)[indices[index].index];
}

TransistorPath::IndexInfo TransistorPath::indexForEndpoint(Endpoint* endpoint) {
    auto it = std::find_if(lines->begin(), lines->end(), 
        [endpoint](Line* line) {
            return line == endpoint->getLine();
        });
    return {
        static_cast<int>(std::distance(lines->begin(), it)),
        endpoint->getIsAtStart()
    };
}

void TransistorPath::expandBackward() {
    auto* prevEndpoint = endpoints[0]->prev();
    if (prevEndpoint) {
        endpoints[0] = prevEndpoint;
        indices.insert(indices.begin(), indexForEndpoint(prevEndpoint));
    } else {
        extendable[0] = false;
    }
}

void TransistorPath::expandForward() {
    indices.push_back(indexForEndpoint(endpoints[1]));
    auto* nextEndpoint = endpoints[1]->next();
    if (nextEndpoint) {
        endpoints[1] = nextEndpoint;
    } else {
        extendable[1] = false;
    }
}

void TransistorPath::merge(TransistorPath* pathB) {
    endpoints[1] = pathB->endpoints[1];
    extendable[1] = pathB->extendable[1];
    indices.insert(indices.end(), pathB->indices.begin(), pathB->indices.end());
}

//void TransistorPath::highlight(Context* context,
//                             const std::function<Vec2(const Vec2&)>& convertToScreen) {
//    for (auto* endpoint : endpoints) {
//        endpoint->highlight(context, convertToScreen);
//    }
//    for (size_t i = 0; i < indices.size(); i++) {
//        lineFromIndex(i)->highlight(context, convertToScreen);
//    }
//}
//
//void TransistorPath::print() {
//    // Assuming there's a highlight function in the global scope
//    highlight(this);
//}

} // namespace ms 