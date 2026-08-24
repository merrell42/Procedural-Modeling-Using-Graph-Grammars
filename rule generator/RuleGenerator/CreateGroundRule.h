#pragma once

class FaceType;
class GraphGrammar;
class Primitives;

// If the primitives contain four corner vertex types that form a ground
// rectangle, create a ground rule and add it to the grammar.
// Returns the ground face type, or nullptr if no ground rule was created.
FaceType* tryCreateGroundRule(GraphGrammar& grammar, Primitives* primitives);
