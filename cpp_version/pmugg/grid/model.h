#pragma once
#include "cell.h"
#include "vec2.h"
#include <vector>
#include <functional>

namespace ms {

class Model {
public:
    Model(int rows, int cols);
    ~Model();

    // Grid properties
    int getRows() const;
    int getCols() const;
    float getCellSize() const;
    void setCellSize(float size);

    // Cell access
    Cell* getCell(int row, int col) const;
    std::vector<Cell*> getCells() const;
    bool isValidCell(int row, int col) const;

    // Cell state management
    void setActive(int row, int col, bool active);
    void setHighlighted(int row, int col, bool highlighted);
    void clearHighlights();

    // Grid operations
    void resize(int rows, int cols);
    void clear();

    // Observer pattern
    void addObserver(const std::function<void()>& observer);
    void removeObserver(const std::function<void()>& observer);
    void notify();

private:
    int rows;
    int cols;
    float cellSize;
    std::vector<std::vector<Cell*>> cells;
    std::vector<std::function<void()>> observers;

    // Helper methods
    void createCells();
    void deleteCells();
    void updateCellPositions();
};

} // namespace ms 