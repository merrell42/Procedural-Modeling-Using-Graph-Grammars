#pragma once
#include <string>
#include <vector>
#include <map>
#include <any>
#include <functional>

namespace ms {

class Node;

class Property {
public:
    Property(const std::string& name, bool required = false);
    virtual ~Property() = default;

    // Common interface
    virtual void setParent(Node* parent);
    virtual void* get() = 0;
    virtual void add(void* element, bool atStart = false) = 0;
    virtual void remove(void* element) = 0;
    virtual void forEachNeighbor(std::function<void(void*)> func) = 0;
    virtual void setChangeHandler(std::function<void()> handler);
    virtual void onChanged();
    virtual void destroy() = 0;
    virtual bool onNodeDestroyed(void* element) = 0;
    virtual void save() = 0;
    virtual void restore() = 0;
    
    const std::string& getName() const;

protected:
    std::string name;
    bool required;
    Node* parent;
    std::function<void()> changeHandler;
};

class SingleProperty : public Property {
public:
    SingleProperty(const std::string& name, bool required = false);
    
    void* get() override;
    void add(void* element, bool atStart = false) override;
    void remove(void* element) override;
    void forEachNeighbor(std::function<void(void*)> func) override;
    void destroy() override;
    bool onNodeDestroyed(void* element) override;
    void save() override;
    void restore() override;

private:
    void* element;
    void* savedElement;
};

class RequiredArray : public Property {
public:
    RequiredArray(const std::string& name, bool required = false, size_t size = 0);
    
    void* get() override;
    void add(void* element, bool atStart = false) override;
    void remove(void* element) override;
    void forEachNeighbor(std::function<void(void*)> func) override;
    void destroy() override;
    bool onNodeDestroyed(void* element) override;
    void save() override;
    void restore() override;
    
    void splice(void* element, size_t index);
    void insert(void* element, size_t index);

private:
    std::vector<void*> array;
    std::vector<void*> savedArray;
    size_t size;
};

class AlternativeArray : public Property {
public:
    AlternativeArray(const std::string& name, bool required = false);
    
    void* get() override;
    void add(void* element, bool atStart = false) override;
    void remove(void* element) override;
    void forEachNeighbor(std::function<void(void*)> func) override;
    void destroy() override;
    bool onNodeDestroyed(void* element) override;
    void save() override;
    void restore() override;
    
    void splice(void* element, size_t index);
    void setOrder(const std::vector<void*>& newOrder);

private:
    std::vector<void*> array;
    std::vector<void*> savedArray;
};

class ValueProperty : public Property {
public:
    ValueProperty(const std::string& name, const std::string& costTerm = "");
    
    void* get() override;
    void add(void* element, bool atStart = false) override;
    void remove(void* element) override;
    void forEachNeighbor(std::function<void(void*)> func) override;
    void destroy() override;
    bool onNodeDestroyed(void* element) override;
    void save() override;
    void restore() override;
    
    void set(const std::any& value);

private:
    void applyCostDelta(float delta);
    
    std::any value;
    std::any savedValue;
    std::string costTerm;
};

} // namespace ms 