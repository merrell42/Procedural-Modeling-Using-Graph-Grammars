#include "decoration_view.h"
#include "dom_utils.h"  // Hypothetical DOM manipulation utility

namespace ms {

DecorationView::DecorationView(const std::string& containerId, DecorationModel* model)
    : containerId(containerId)
    , model(model) {
    
    model->addObserver([this]() { this->notify(); });
    draw();
}

DecorationView::~DecorationView() {
    // Clean up any DOM elements if necessary
}

void DecorationView::draw() {
    // Clear existing content
    auto* container = DOMUtils::getElementById(containerId);
    if (!container) return;

    container->innerHTML = "";

    // Draw each brush collection
    for (size_t i = 0; i < model->brushCollections.size(); ++i) {
        drawBrushCollection(model->brushCollections[i], static_cast<int>(i));
    }
}

void DecorationView::notify() {
    draw();
}

void DecorationView::drawBrushCollection(const BrushCollection& collection, int index) {
    auto* container = DOMUtils::getElementById(containerId);
    if (!container) return;

    // Create collection container
    auto* collectionDiv = DOMUtils::createElement("div");
    collectionDiv->className = "brush-collection";
    container->appendChild(collectionDiv);

    // Create brushes
    for (auto* brush : collection.getBrushes()) {
        createBrushElement(brush);
    }
}

void DecorationView::createBrushElement(Brush* brush) {
    auto* container = DOMUtils::getElementById(containerId);
    if (!container) return;

    // Create brush element
    auto* brushDiv = DOMUtils::createElement("div");
    brushDiv->className = "brush";
    brushDiv->style.backgroundColor = brush->getFillStyle();
    brushDiv->style.borderColor = brush->getStrokeStyle();

    // Add click handler
    brushDiv->onclick = [brush, this]() {
        // Handle brush selection
        // Update model state
        model->notify();
    };

    container->appendChild(brushDiv);
}

} // namespace ms 