#include "edge_selector.h"
#include "selector.h"
#include "intersector.h"

namespace ms {

EdgeSelector::EdgeSelector(Driver* driver) : driver(driver) {}

void EdgeSelector::select(const Shape& selection, bool add, bool subtract) {
    Shape* selectedShape = driver->selectedShape();
    if (!selectedShape) {
        return;
    }

    auto vertices = selectedShape->getVertices();
    std::vector<Vertex*> interiorVertices;
    
    for (auto* v : vertices) {
        bool isInside = selection.isInside(v);
        if (isInside) {
            interiorVertices.push_back(v);
        }
        Selector::updateSelectable(v, add, subtract, isInside);
    }
}

Selectable* EdgeSelector::findClose(const std::vector<Selectable*>& selectables, 
                                  const Vec2& position, float scale) {
    for (auto* selectable : selectables) {
        if (selectable->selectType() == SelectableTypes::EXAMPLE_SHAPE) {
            auto* leftEdge = selectable->nearestLeftEdge(position);
            auto* endpoint = leftEdge ? leftEdge->endpoint : nullptr;
            auto* face = endpoint ? endpoint->getFace() : nullptr;
            if (face && face->getArea() > 0) {
                return face;
            }
        } else {
            if (selectable->isClose(position, scale)) {
                return selectable;
            }
        }
    }
    return nullptr;
}

Vertex* EdgeSelector::findVertex(const Vec2& position, float scale) {
    if (!driver->selectedShape()) {
        return nullptr;
    }
    return static_cast<Vertex*>(findClose(driver->getVertices(), position, scale));
}

Selectable* EdgeSelector::pointSelect(const Vec2& position, const SelectOptions& options) {
    bool add = options.add;
    bool subtract = options.subtract;
    float scale = options.scale;

    auto selectables = driver->getSelectables(options);
    auto* selectable = findClose(selectables, position, scale);

    if (selectable) {
        if (!selectable->getSelected()) {
            if (!add) {
                driver->selectNone(false);
            }
            selectable->select();
        } else if (subtract) {
            selectable->deselect();
        }
    } else if (!add && !subtract) {
        driver->selectNone();
    }
    
    return selectable;
}

} // namespace ms 