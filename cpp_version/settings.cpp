#include "pch.h"
#include "settings.h"

// TODO: Remove the unused settings.
// Most of these settings are no longer used.
Json globalSettings = {
    {"Extents", vector<int>{30, 20, 10}},
    {"Incremental Mutation", true},
    {"Use Boundary Cells", false},
    {"Model Piece Example", "ms.ESCHER_EXAMPLE"},
    {"Redraw Time", 0.1f},
    {"Allow Rotations", false},
    {"Bend Edges", false},
    {"Careful Bending", true},
    {"Bend Time Start", 0},
    {"Allow Scaling", false},
    {"Import Scale", 1.0f},
    {"Many Rigid Bent Options", true},
    {"Many Flexible Bent Options", false},
    {"Remove Good Vertices", false},
    {"Bend Multiplier", 1},
    {"Snap Grid", true},
    {"Snap Grid Spacing", 20},
    {"Major Grid Spacing", 40},
    {"Snap Angle", 20},
    {"Mutator Effort Limit", 20},
    {"Prefer Ground", 0.9f},
    {"Empty Border", true},
    // Optimizer Metropolis temperature. exp(Beta * cost-delta) is the
    // acceptance probability — Beta=0 is always-accept (bit-identical to
    // the pre-optimizer behavior), Beta=1 is strong rejection. The default
    // Beta=0.01 is the local min on the bundled corpus: enough rejection
    // to visibly shape output (Square Filled 4 -> 30 faces, Docks
    // 163 -> 232) while keeping the cost-machinery + reject deep-copy
    // overhead to ~14% wall-clock vs always-accept. Higher Beta values
    // increase rejection rate but pay more reject deep-copy cost; lower
    // Beta accepts more bad mutations and lets the algorithm wander into
    // expensive states. Override per-grammar via the grammar JSON.
    {"Beta", 0.01},
    {"Snapping Effort Limit", 100},
    {"Bad Vertex", 10},
    {"HalfEdge Conflict", 1},
    {"HalfEdge Conflict Base", 1.1f},
    {"Flexible Edge Length", 50},
    {"Rigid Edge Length", 500},
    {"Ideal Length", 0.5f},
    {"Face", 5},
    {"Completion", 4},
    {"Valence", 0},
    {"Component", 20},
    {"State Density", 2},
    {"Component Scale", 10},
    {"Component Base", 1.5f},
    {"Face Base", 1.1f},
    {"Visibility", 0},
    {"Closible HalfEdge Index", 1},
    {"Closible Angle", 200},
    {"Desired Edges", 200},
    {"Desired Edges Cost", 10000},
    // Optimizer cost-function targets. `Desired Lines` is the line count
    // the optimizer biases output toward; `Desired Lines Cost` scales the
    // penalty for being off-target. Sensible defaults from a small corpus
    // of grammars; override per-grammar via the grammar JSON if needed.
    {"Desired Lines", 200},
    {"Desired Lines Cost", 10000},
    {"Desired Vertex Weight", 100},
    {"Free Vertices", true},
    {"Kill Junctions", true},
    {"Full Move", true},
    {"Modify center", false},
    {"Immutable Ground", true},
    {"Edge Weld Probability", 0.7f},
    {"Node Debug", -1},
    {"Debug Alerts", false},
    {"Max Time", 10},
    {"Generations", 5},
    {"Max Connectors", 100},
    {"Winding Enabled", true},
    {"Stubs Enabled", false},
    {"Match Backwards", true},
    {"Use Chain Map", false},
    {"Monotonic", true},
    {"Super Edges", true},
    {"Show Inactives", false},
    {"Multiple Attachees", false},
    {"Grounded", true},
    {"Winding Penalty", 1},
    {"Winding Penalty New", 10},
    {"New Seed", false},
    {"Seed", 1},
    {"Task Stop", -1},
    {"Task Step", 1},
    {"Debug Each Task", false},
    {"Fast Matrix Math", true},
    {"Report Timings", true},
    {"Show Grid", false},
    {"High Smoothing", true},
    {"Use WebGL", false},
    {"Show Faces", true},
    {"Cut Holes", true},
    {"Fewer Start Productions", false},
    {"Example View", true},
    {"Use Graph", true},
    {"Pause when finished", false}
};

