#pragma once
#include "brush.h"
#include <vector>
#include <memory>

namespace ms {

class BrushCollection {
public:
    BrushCollection() = default;
    ~BrushCollection();

    void addBrush(Brush* brush);
    void removeBrush(Brush* brush);
    const std::vector<Brush*>& getBrushes() const;
    void clear();

private:
    std::vector<Brush*> brushes;
};

} // namespace ms 