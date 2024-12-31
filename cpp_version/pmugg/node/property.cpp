#include "property.h"
#include "node.h"
#include <algorithm>
#include <stdexcept>

namespace ms {

// Base Property implementation
Property::Property(const std::string& name, bool required)
    : name(name)
    , required(required)
    , parent(nullptr)
    , changeHandler([](){}) {}

void Property::setParent(Node* p) {
    parent = p;
}

void Property::setChangeHandler(std::function<void()> handler) {
    changeHandler = handler;
}

void Property::onChanged() {
    parent->onChanged();
    changeHandler();
}

const std::string& Property::getName() const {
    return name;
}

// SingleProperty implementation
SingleProperty::SingleProperty(const std::string& name, bool required)
    : Property(name, required)
    , element(nullptr)
    , savedElement(nullptr) {}

void* SingleProperty::get() {
    return element;
}

void SingleProperty::add(void* newElement, bool atStart) {
    if (element == newElement) {
        return;
    }
    if (element) {
        static_cast<Node*>(element)->disconnect(parent->getElement());
    }
    element = newElement;
    onChanged();
}

void SingleProperty::remove(void* oldElement) {
    if (element != oldElement) {
        throw std::runtime_error("Removing an element that does not exist");
    }
    element = nullptr;
    onChanged();
}

void SingleProperty::forEachNeighbor(std::function<void(void*)> func) {
    if (element) {
        func(element);
    }
}

void SingleProperty::destroy() {}

bool SingleProperty::onNodeDestroyed(void* destroyedElement) {
    remove(destroyedElement);
    return required;
}

void SingleProperty::save() {
    savedElement = element;
}

void SingleProperty::restore() {
    element = savedElement;
}

// RequiredArray implementation
RequiredArray::RequiredArray(const std::string& name, bool required, size_t size)
    : Property(name, required)
    , size(size) {
    array.resize(size);
}

void* RequiredArray::get() {
    return &array;
}

void RequiredArray::add(void* element, bool atStart) {
    if (size == 0) {
        if (atStart) {
            array.insert(array.begin(), element);
        } else {
            array.push_back(element);
        }
    } else {
        size_t index = 0;
        while (index < size && array[index]) {
            index++;
        }
        if (index >= size) {
            throw std::runtime_error("Out of bounds in required array");
        }
        array[index] = element;
    }
    onChanged();
}

void RequiredArray::remove(void* element) {
    if (size > 0) {
        auto it = std::find(array.begin(), array.end(), element);
        if (it == array.end()) {
            throw std::runtime_error("Removing an element that does not exist");
        }
        *it = nullptr;
    } else {
        auto it = std::find(array.begin(), array.end(), element);
        if (it == array.end()) {
            throw std::runtime_error("Removing an element that does not exist");
        }
        array.erase(it);
    }
    onChanged();
}

void RequiredArray::forEachNeighbor(std::function<void(void*)> func) {
    for (auto* elem : array) {
        if (elem) {
            func(elem);
        }
    }
}

void RequiredArray::destroy() {}

bool RequiredArray::onNodeDestroyed(void* element) {
    remove(element);
    if (required) {
        return std::none_of(array.begin(), array.end(), 
                           [](void* elem) { return elem != nullptr; });
    }
    return false;
}

void RequiredArray::save() {
    savedArray = array;
}

void RequiredArray::restore() {
    array = savedArray;
}

void RequiredArray::splice(void* element, size_t index) {
    array.insert(array.begin() + index, element);
    onChanged();
}

void RequiredArray::insert(void* element, size_t index) {
    if (index >= size) {
        throw std::runtime_error("Index out of bound in required array");
    }
    array[index] = element;
    onChanged();
}

// AlternativeArray implementation
AlternativeArray::AlternativeArray(const std::string& name, bool required)
    : Property(name, required) {}

void* AlternativeArray::get() {
    return &array;
}

void AlternativeArray::add(void* element, bool atStart) {
    if (std::find(array.begin(), array.end(), element) != array.end()) {
        return;
    }
    array.push_back(element);
    onChanged();
}

void AlternativeArray::remove(void* element) {
    auto it = std::find(array.begin(), array.end(), element);
    if (it == array.end()) {
        throw std::runtime_error("Removing an element that does not exist");
    }
    array.erase(it);
    onChanged();
}

void AlternativeArray::forEachNeighbor(std::function<void(void*)> func) {
    for (auto* elem : array) {
        func(elem);
    }
}

void AlternativeArray::destroy() {}

bool AlternativeArray::onNodeDestroyed(void* element) {
    auto it = std::find(array.begin(), array.end(), element);
    if (it != array.end()) {
        array.erase(it);
    }
    onChanged();
    return required && array.empty();
}

void AlternativeArray::save() {
    savedArray = array;
}

void AlternativeArray::restore() {
    array = savedArray;
}

void AlternativeArray::splice(void* element, size_t index) {
    array.insert(array.begin() + index, element);
    onChanged();
}

void AlternativeArray::setOrder(const std::vector<void*>& newOrder) {
    array = newOrder;
}

// ValueProperty implementation
ValueProperty::ValueProperty(const std::string& name, const std::string& costTerm)
    : Property(name)
    , costTerm(costTerm) {}

void* ValueProperty::get() {
    return &value;
}

void ValueProperty::add(void*, bool) {
    throw std::runtime_error("Cannot add to ValueProperty");
}

void ValueProperty::remove(void*) {
    throw std::runtime_error("Cannot remove from ValueProperty");
}

void ValueProperty::forEachNeighbor(std::function<void(void*)>) {}

void ValueProperty::destroy() {
    if (!costTerm.empty() && value.has_value()) {
        applyCostDelta(-std::any_cast<float>(value));
    }
}

bool ValueProperty::onNodeDestroyed(void*) {
    return false;
}

void ValueProperty::save() {
    savedValue = value;
}

void ValueProperty::restore() {
    value = savedValue;
}

void ValueProperty::set(const std::any& newValue) {
    float delta = 0;
    if (!costTerm.empty()) {
        if (value.has_value()) {
            delta -= std::any_cast<float>(value);
        }
        if (newValue.has_value()) {
            delta += std::any_cast<float>(newValue);
        }
        applyCostDelta(delta);
    }
    value = newValue;
}

void ValueProperty::applyCostDelta(float delta) {
    if (delta == 0) {
        return;
    }
    // TODO: Implement cost tracking system
}

} // namespace ms 