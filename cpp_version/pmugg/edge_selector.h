#pragma once
#include "driver.h"
#include "shape.h"
#include "vec2.h"

namespace ms {

class EdgeSelector {
public:
    explicit EdgeSelector(Driver* driver);
    
    void select(const Shape& selection, bool add, bool subtract);
    Vertex* findVertex(const Vec2& position, float scale = 1.0f);
    Selectable* pointSelect(const Vec2& position, const SelectOptions& options);
    
    static Selectable* findClose(const std::vector<Selectable*>& selectables, 
                               const Vec2& position, float scale);

private:
    Driver* driver;
};

} // namespace ms 