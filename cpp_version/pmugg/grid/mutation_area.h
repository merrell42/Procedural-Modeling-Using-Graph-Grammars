#pragma once
#include "model.h"
#include "vec2.h"
#include <vector>

namespace ms {

class MutationArea {
public:
    MutationArea(Model* model);

    // Area management
    void start(int row, int col);
    void update(int row, int col);
    void end();
    bool isActive() const;

    // Area properties
    int getStartRow() const;
    int getStartCol() const;
    int getEndRow() const;
    int getEndCol() const;
    std::vector<Cell*> getSelectedCells() const;

    // Area operations
    void clear();
    void fill();
    void invert();

private:
    Model* model;
    bool active;
    int startRow;
    int startCol;
    int endRow;
    int endCol;

    // Helper methods
    void updateHighlights();
    void forEachCell(const std::function<void(Cell*)>& callback) const;
    std::pair<int, int> getNormalizedBounds() const;
};

} // namespace ms 