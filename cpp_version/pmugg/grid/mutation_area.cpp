#include "mutation_area.h"
#include <algorithm>

namespace ms {

MutationArea::MutationArea(Model* model)
    : model(model)
    , active(false)
    , startRow(0)
    , startCol(0)
    , endRow(0)
    , endCol(0) {
}

void MutationArea::start(int row, int col) {
    active = true;
    startRow = row;
    startCol = col;
    endRow = row;
    endCol = col;
    updateHighlights();
}

void MutationArea::update(int row, int col) {
    if (!active) return;
    
    endRow = row;
    endCol = col;
    updateHighlights();
}

void MutationArea::end() {
    active = false;
    model->clearHighlights();
}

bool MutationArea::isActive() const {
    return active;
}

int MutationArea::getStartRow() const {
    return startRow;
}

int MutationArea::getStartCol() const {
    return startCol;
}

int MutationArea::getEndRow() const {
    return endRow;
}

int MutationArea::getEndCol() const {
    return endCol;
}

std::vector<Cell*> MutationArea::getSelectedCells() const {
    std::vector<Cell*> selectedCells;
    forEachCell([&selectedCells](Cell* cell) {
        selectedCells.push_back(cell);
    });
    return selectedCells;
}

void MutationArea::clear() {
    forEachCell([](Cell* cell) {
        cell->setActive(false);
    });
}

void MutationArea::fill() {
    forEachCell([](Cell* cell) {
        cell->setActive(true);
    });
}

void MutationArea::invert() {
    forEachCell([](Cell* cell) {
        cell->setActive(!cell->isActive());
    });
}

void MutationArea::updateHighlights() {
    model->clearHighlights();
    
    if (!active) return;

    forEachCell([](Cell* cell) {
        cell->setHighlighted(true);
    });
}

void MutationArea::forEachCell(const std::function<void(Cell*)>& callback) const {
    auto [minRow, maxRow] = getNormalizedBounds();
    
    for (int row = minRow.first; row <= maxRow.first; ++row) {
        for (int col = minRow.second; col <= maxRow.second; ++col) {
            if (Cell* cell = model->getCell(row, col)) {
                callback(cell);
            }
        }
    }
}

std::pair<int, int> MutationArea::getNormalizedBounds() const {
    int minRow = std::min(startRow, endRow);
    int maxRow = std::max(startRow, endRow);
    int minCol = std::min(startCol, endCol);
    int maxCol = std::max(startCol, endCol);

    // Clamp to grid bounds
    minRow = std::max(0, minRow);
    maxRow = std::min(model->getRows() - 1, maxRow);
    minCol = std::max(0, minCol);
    maxCol = std::min(model->getCols() - 1, maxCol);

    return {{minRow, minCol}, {maxRow, maxCol}};
}

} // namespace ms 