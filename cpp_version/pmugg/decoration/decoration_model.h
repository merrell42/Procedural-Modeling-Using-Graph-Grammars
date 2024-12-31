#pragma once
#include "brush_collection.h"
#include "example_shape.h"
#include <vector>
#include <functional>

namespace ms {

class DecorationModel {
public:
    explicit DecorationModel(ExampleShape* exampleShape);
    ~DecorationModel();

    std::vector<BrushCollection> brushCollections;
    ExampleShape* exampleShape;

    void notify();
    void addObserver(const std::function<void()>& observer);
    void removeObserver(const std::function<void()>& observer);

private:
    std::vector<std::function<void()>> observers;
};

} // namespace ms 