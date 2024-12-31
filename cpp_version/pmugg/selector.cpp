#include "selector.h"
#include "view.h"
#include "canvas.h"
#include "context.h"

namespace ms {

Brush* Selector::brush = nullptr;

Selector::Selector(Driver* driver, InternalSelector* internalSelector)
    : driver(driver)
    , internalSelector(internalSelector)
    , lassoEnabled(false) {
    
    selection.active = false;
    selection.add = false;
    selection.subtract = false;
}

Selector::~Selector() {
    // Clean up any resources
}

void Selector::register_(View* view) {
    addObserver([view]() { view->redraw(nullptr); });
}

void Selector::setLasso(bool enabled) {
    lassoEnabled = enabled;
}

void Selector::selectAll() {
    driver->selectAll();
    notify();
}

void Selector::selectNone() {
    driver->selectNone();
    notify();
}

void Selector::cancel() {
    if (selection.active) {
        selection.active = false;
        notify();
    }
}

void Selector::notify() {
    for (const auto& observer : observers) {
        observer();
    }
}

void Selector::onMouseMove(const MouseEvent& event) {
    if (selection.active) {
        selection.current = event.position;
        notify();
    }
}

void Selector::onMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::LEFT) {
        return;
    }

    selection.start = event.position;
    selection.current = event.position;
    selection.active = true;
    selection.add = event.shiftKey;
    selection.subtract = event.altKey;

    if (!selection.add && !selection.subtract) {
        driver->selectNone();
    }

    notify();
}

void Selector::onMouseUp(const MouseEvent& event) {
    if (!selection.active || event.button != MouseButton::LEFT) {
        return;
    }

    if (selection.start.distanceTo(event.position) < 5) {
        // Point selection
        SelectOptions options;
        options.add = selection.add;
        options.subtract = selection.subtract;
        options.scale = event.scale;
        pointSelect(event.position, options);
    } else {
        // Area selection
        applySelection();
    }

    selection.active = false;
    notify();
}

void Selector::addObserver(const std::function<void()>& observer) {
    observers.push_back(observer);
}

void Selector::removeObserver(const std::function<void()>& observer) {
    // Remove observer by comparing function pointers
    observers.erase(
        std::remove_if(observers.begin(), observers.end(),
            [&](const auto& obs) { 
                return obs.target<void()>() == observer.target<void()>(); 
            }),
        observers.end()
    );
}

void Selector::drawSelection(const Vec2& start, const Vec2& end) {
    if (!brush) {
        return;
    }

    auto* context = Canvas::getContext();
    context->save();
    
    if (lassoEnabled) {
        // Draw lasso selection
        context->beginPath();
        context->moveTo(start.x, start.y);
        context->lineTo(end.x, end.y);
        context->setStrokeStyle(brush->getColor());
        context->setLineWidth(2);
        context->stroke();
    } else {
        // Draw rectangle selection
        float x = std::min(start.x, end.x);
        float y = std::min(start.y, end.y);
        float width = std::abs(end.x - start.x);
        float height = std::abs(end.y - start.y);

        context->setStrokeStyle(brush->getColor());
        context->setLineWidth(2);
        context->strokeRect(x, y, width, height);

        context->setFillStyle(brush->getColor());
        context->setGlobalAlpha(0.1);
        context->fillRect(x, y, width, height);
    }

    context->restore();
}

void Selector::clearSelection() {
    driver->selectNone();
    notify();
}

void Selector::applySelection() {
    if (lassoEnabled) {
        // Apply lasso selection
        std::vector<Vec2> points = {selection.start, selection.current};
        driver->selectLasso(points, selection.add, selection.subtract);
    } else {
        // Apply rectangle selection
        float x = std::min(selection.start.x, selection.current.x);
        float y = std::min(selection.start.y, selection.current.y);
        float width = std::abs(selection.current.x - selection.start.x);
        float height = std::abs(selection.current.y - selection.start.y);

        Rect selectionRect(x, y, width, height);
        driver->selectRect(selectionRect, selection.add, selection.subtract);
    }
}

Vertex* Selector::findVertex(const Vec2& position, float scale) {
    return internalSelector->findVertex(position, scale);
}

Selectable* Selector::pointSelect(const Vec2& position, const SelectOptions& options) {
    auto* selectable = internalSelector->pointSelect(position, options);
    notify();
    return selectable;
}

} // namespace ms 