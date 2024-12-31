#pragma once
#include <map>
#include <string>
#include <vector>
#include "collection.h"

namespace ms {

class Model;

class NodeStats {
public:
    NodeStats();
    ~NodeStats();

    // Model operations
    void setModel(Model* model);
    Model* getModel() const;

    // Node operations
    void addNode(Node* node);
    void removeNode(Node* node);
    void addVertex(Node* node);
    void removeVertex(Node* node);
    void updateFace(Face* face, bool unlooped);

    // Collection getters
    std::vector<Vertex*> getBadVertices() const;
    std::vector<Face*> getUnloopedFaces() const;
    std::vector<void*> getElements(const std::string& name) const;
    int getCount(const std::string& name) const;

    // Change tracking
    void addToChangeList(Node* node);
    void apply(std::function<void(Node*)> func);
    void save();
    void restore();
    void resetCostChange();

    // Verification
    void verifyAll();
    int componentCount() const;

    // Static access
    static NodeStats* get();
    static void verifyAll_();

private:
    std::map<std::string, std::map<int, Node*>> nodes;
    std::map<std::string, int> count;
    std::vector<Node*> changeList;
    std::map<std::string, float> costChange;
    Model* model;
    Collection badCollection;
    Collection unloopedCollection;

    static const std::vector<std::string> nodeTypes;
    static const std::vector<std::string> costTerms;
};

} // namespace ms 