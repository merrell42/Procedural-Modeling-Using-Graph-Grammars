#include "model.h"
#include <algorithm>

namespace ms {

Model::Model(int rows, int cols)
    : rows(rows)
    , cols(cols)
    , cellSize(20.0f) {
    createCells();
}

Model::~Model() {
    deleteCells();
}

int Model::getRows() const {
    return rows;
}

int Model::getCols() const {
    return cols;
}

float Model::getCellSize() const {
    return cellSize;
}

void Model::setCellSize(float size) {
    cellSize = size;
    updateCellPositions();
    notify();
}

Cell* Model::getCell(int row, int col) const {
    if (!isValidCell(row, col)) {
        return nullptr;
    }
    return cells[row][col];
}

std::vector<Cell*> Model::getCells() const {
    std::vector<Cell*> allCells;
    allCells.reserve(rows * cols);
    for (const auto& row : cells) {
        allCells.insert(allCells.end(), row.begin(), row.end());
    }
    return allCells;
}

bool Model::isValidCell(int row, int col) const {
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

void Model::setActive(int row, int col, bool active) {
    if (auto* cell = getCell(row, col)) {
        cell->setActive(active);
        notify();
    }
}

void Model::setHighlighted(int row, int col, bool highlighted) {
    if (auto* cell = getCell(row, col)) {
        cell->setHighlighted(highlighted);
        notify();
    }
}

void Model::clearHighlights() {
    for (auto& row : cells) {
        for (auto* cell : row) {
            cell->setHighlighted(false);
        }
    }
    notify();
}

void Model::resize(int newRows, int newCols) {
    deleteCells();
    rows = newRows;
    cols = newCols;
    createCells();
    notify();
}

void Model::clear() {
    for (auto& row : cells) {
        for (auto* cell : row) {
            cell->setActive(false);
            cell->setHighlighted(false);
        }
    }
    notify();
}

void Model::addObserver(const std::function<void()>& observer) {
    observers.push_back(observer);
}

void Model::removeObserver(const std::function<void()>& observer) {
    observers.erase(
        std::remove_if(observers.begin(), observers.end(),
            [&](const auto& obs) {
                return obs.target<void()>() == observer.target<void()>();
            }),
        observers.end()
    );
}

void Model::notify() {
    for (const auto& observer : observers) {
        observer();
    }
}

void Model::createCells() {
    cells.resize(rows);
    for (int i = 0; i < rows; ++i) {
        cells[i].resize(cols);
        for (int j = 0; j < cols; ++j) {
            cells[i][j] = new Cell(i, j);
        }
    }
    updateCellPositions();
}

void Model::deleteCells() {
    for (auto& row : cells) {
        for (auto* cell : row) {
            delete cell;
        }
    }
    cells.clear();
}

void Model::updateCellPositions() {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            cells[i][j]->setPosition(Vec2(j * cellSize, i * cellSize));
            cells[i][j]->setSize(cellSize);
        }
    }
}

} // namespace ms 