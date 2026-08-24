#pragma once

class Primitives;
class ProductionRule;

// If the primitives contain four corner vertex types that form a ground
// rectangle, return a ground rule. Otherwise return nullptr.
ProductionRule* createGroundRule(Primitives* primitives);
