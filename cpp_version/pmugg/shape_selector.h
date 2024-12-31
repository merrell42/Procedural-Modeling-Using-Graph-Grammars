#pragma once
#include "internal_selector.h"
#include "example_shape.h"
#include "vec2.h"
#include <vector>

namespace ms {

class ShapeSelector : public InternalSelector {
public:
    explicit ShapeSelector(ExampleShape* exampleShape);

    Vertex* findVertex(const Vec2& position, float scale = 1.0f) override;
    Selectable* pointSelect(const Vec2& position, const SelectOptions& options) override;

    static Selectable* findClose(const std::vector<Selectable*>& selectables,
                               const Vec2& position,
                               float scale);

private:
    ExampleShape* exampleShape;
};

} // namespace ms 