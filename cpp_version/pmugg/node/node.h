#pragma once
#include <string>
#include <map>
#include <functional>
#include "property.h"
#include "../shape/vec2.h"

namespace ms {

class Stats;

class Node {
public:
    Node(void* element, Stats* stats, const std::string& name, 
         const std::map<std::string, Property*>& properties);
    ~Node();

    // Getters
    std::string getName() const;
    int getId() const;
    Stats* getStats() const;
    void* getElement() const;
    bool isDestroyed() const;

    // Property operations
    bool has(const std::string& name) const;
    template<typename T>
    T get(const std::string& name) const;
    void add(const std::string& name, void* element, bool atStart = false);
    void remove(const std::string& name, void* element);
    void setChangeHandler(const std::string& name, std::function<void()> handler);
    void setValue(const std::string& name, const std::any& value);
    void setArray(const std::vector<void*>& elementsB);
    void spliceInsert(int indexA, int indexB, void* elementB);
    void doubleInsert(int indexA, int indexB, void* elementB);
    void insert(void* elementB, int index);
    void splice(void* elementB, int index);
    void connect(void* elementB, bool atStart = false);
    void disconnect(void* elementB);

    // State management
    void onChanged();
    void destroy();
    void save();
    void restoreDestroyed();
    void restoreCreated();
    void print() const;

    // Handlers
    void setDestroyHandler(std::function<void()> handler);
    void setUndestroyHandler(std::function<void()> handler);

private:
    void destroy_();
    void onNodeDestroyed(Node* node);
    void forEachNeighbor(std::function<void(void*)> func);

    static int count;
    
    void* element;
    Stats* stats;
    std::string name;
    int id;
    std::map<std::string, Property*> properties;
    bool destroyed;
    bool wasDestroyed;
    bool changed;
    std::function<void()> destroyHandler;
    std::function<void()> undestroyHandler;
};

} // namespace ms 