#include "decoration_model.h"
#include <algorithm>

namespace ms {

DecorationModel::DecorationModel(ExampleShape* exampleShape)
    : exampleShape(exampleShape) {
    // Initialize with default collections
    brushCollections.resize(2);
}

DecorationModel::~DecorationModel() {
    // Clean up brush collections
    brushCollections.clear();
}

void DecorationModel::notify() {
    for (const auto& observer : observers) {
        observer();
    }
}

void DecorationModel::addObserver(const std::function<void()>& observer) {
    observers.push_back(observer);
}

void DecorationModel::removeObserver(const std::function<void()>& observer) {
    observers.erase(
        std::remove_if(observers.begin(), observers.end(),
            [&](const auto& obs) {
                return obs.target<void()>() == observer.target<void()>();
            }),
        observers.end()
    );
}

} // namespace ms 