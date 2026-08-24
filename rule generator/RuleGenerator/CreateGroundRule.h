#pragma once

class GraphGrammar;
class Primitives;

// If the primitives contain four corner vertex types that form a ground
// rectangle, create a ground rule and add it to the grammar.
void tryCreateGroundRule(GraphGrammar& grammar, Primitives* primitives);
