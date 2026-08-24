#pragma once
#include <vector>
#include <memory>
#include <map>
#include <iosfwd>
#include "third_party/json.h"
#include "primitives/face_type.h"
#include "primitives/primitives.h"
#include "graph_morphism/morphism.h"

using Json = nlohmann::json;

class Graph;  
class ProductionRule;
class Primitives;
class EdgeType;
class VertexType;

// Everything needed to apply a production rule. The start and end
// graphs and morphism to the graph drawing.
struct Production {
    Graph* startGraph;
    Graph* endGraph;
    Morphism* morphism;
    bool ground;
};

// A graph grammar is a set of production rules that are used to modify the graph drawing.
class GraphGrammar {
public:
    GraphGrammar();
    GraphGrammar(Primitives* primitives);
    ~GraphGrammar();
    static GraphGrammar* import(const Json& json);
    Json exportJson() const;

    // Add a production rule. Takes ownership of the rule.
    void addRule(ProductionRule* rule);
    void addStarterRule(ProductionRule* rule);
    void addGroundRule(ProductionRule* rule);
    void prepareGroundPlane(FaceType* face);

    // Serialize the whole grammar (primitives + rules + emptyGraph) to a binary stream.
    // The output of serialize() can be replayed by deserialize() to obtain an
    // equivalent GraphGrammar without re-parsing JSON. Suitable for on-disk caches.
    void serialize(std::ostream& out) const;
    static GraphGrammar* deserialize(std::istream& in);

    // Get a production rule.
    Production getProduction();
    Production getRemovalProduction();
    Production getStarterProduction(bool useGround);

    bool isGrounded() const;
    bool hasStarterRules(bool useGround) const;
    FaceType* getGroundFaceType() const;

    // Get the number of dimensions of the graph.
    int getDims() const { return primitives->dims; }

private:
    // The production rules.
    // Starter rules have an empty start graph.
    vector<ProductionRule*> starterRules;
    // Normal rules have a non-empty start graph.
    vector<ProductionRule*> rules;
    // Ground rules are like starter rules, but just for creating the
    // ground plane on the first iteration.
    vector<ProductionRule*> groundRules;

    Graph* emptyGraph;
    Primitives* primitives;

    // True if there is a ground plane for this model.
    bool grounded;
    FaceType* groundFace = nullptr;
};

