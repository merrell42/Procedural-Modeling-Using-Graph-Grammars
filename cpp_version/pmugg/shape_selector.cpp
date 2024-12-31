#include "shape_selector.h"
#include "selectable.h"
#include "shape.h"
#include "face.h"

namespace ms {

ShapeSelector::ShapeSelector(ExampleShape* exampleShape)
    : exampleShape(exampleShape) {
}

Vertex* ShapeSelector::findVertex(const Vec2& position, float scale) {
    if (!exampleShape->selectedShape) {
        return nullptr;
    }
    return static_cast<Vertex*>(findClose(exampleShape->selectedShape->getVertices(), 
                                        position, scale));
}

Selectable* ShapeSelector::pointSelect(const Vec2& position, const SelectOptions& options) {
    bool add = options.add;
    bool subtract = options.subtract;
    float scale = options.scale;

    auto selectables = exampleShape->getSelectables(options);
    auto* selectable = findClose(selectables, position, scale);

    if (selectable) {
        if (!selectable->getSelected()) {
            if (!add) {
                exampleShape->selectNone(false);
            }
            selectable->select();
        } else if (subtract) {
            selectable->deselect();
        }
    } else if (!add && !subtract) {
        exampleShape->selectNone();
    }

    return selectable;
}

Selectable* ShapeSelector::findClose(const std::vector<Selectable*>& selectables,
                                   const Vec2& position,
                                   float scale) {
    for (auto* selectable : selectables) {
        if (selectable->selectType() == SelectableTypes::EXAMPLE_SHAPE) {
            // Special handling for example shapes
            auto* shape = static_cast<Shape*>(selectable);
            auto* leftEdge = shape->nearestLeftEdge(position);
            if (leftEdge) {
                auto* endpoint = leftEdge->endpoint;
                if (endpoint) {
                    auto* face = endpoint->getFace();
                    if (face && face->getArea() > 0) {
                        return face;
                    }
                }
            }
        } else {
            // Normal distance check for other selectables
            if (selectable->isClose(position, scale)) {
                return selectable;
            }
        }
    }
    return nullptr;
}

} // namespace ms 