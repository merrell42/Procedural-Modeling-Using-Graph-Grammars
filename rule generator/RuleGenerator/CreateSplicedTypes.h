#pragma once

class Primitives;

// Remove any spliced vertex types and spliced edge types.
void filterSplicedTypes(Primitives* primitives);

// Create spliced vertex types and spliced edge types and add them
// to the primitives.
void createSplicedTypes(Primitives* primitives);
