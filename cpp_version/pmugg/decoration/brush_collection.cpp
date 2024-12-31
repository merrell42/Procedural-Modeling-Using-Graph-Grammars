#include "brush_collection.h"
#include <algorithm>

namespace ms {

BrushCollection::~BrushCollection() {
    clear();
}

void BrushCollection::addBrush(Brush* brush) {
    brushes.push_back(brush);
}

void BrushCollection::removeBrush(Brush* brush) {
    auto it = std::find(brushes.begin(), brushes.end(), brush);
    if (it != brushes.end()) {
        brushes.erase(it);
    }
}

const std::vector<Brush*>& BrushCollection::getBrushes() const {
    return brushes;
}

void BrushCollection::clear() {
    for (auto* brush : brushes) {
        delete brush;
    }
    brushes.clear();
}

} // namespace ms 