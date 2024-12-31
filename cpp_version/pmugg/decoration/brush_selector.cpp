#include "brush_selector.h"
#include "dom_utils.h"

namespace ms {

BrushSelector::BrushSelector(const std::string& containerId, DecorationModel* model)
    : containerId(containerId)
    , model(model) {
    
    model->addObserver([this]() { this->notify(); });
    draw();
}

BrushSelector::~BrushSelector() {
    // Clean up any DOM elements if necessary
}

void BrushSelector::draw() {
    // Clear existing content
    auto* container = DOMUtils::getElementById(containerId);
    if (!container) return;

    container->innerHTML = "";

    // Draw each brush collection
    for (size_t i = 0; i < model->brushCollections.size(); ++i) {
        drawBrushCollection(model->brushCollections[i], static_cast<int>(i));
    }
}

void BrushSelector::notify() {
    draw();
}

void BrushSelector::drawBrushCollection(const BrushCollection& collection, int index) {
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

void BrushSelector::createBrushElement(Brush* brush) {
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