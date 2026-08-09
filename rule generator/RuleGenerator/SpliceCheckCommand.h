#pragma once

#include <string>

// Verifies spliced-vertex edge-state domain, left/right FaceData selection,
// and face propagation on spliced edges. Returns 0 on success.
int runSpliceCheck(
	const std::string& primitivesPath,
	const std::string& templatesPath
);
