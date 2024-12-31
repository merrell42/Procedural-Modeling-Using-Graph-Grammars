#include "setup.h"
#include "global_settings.h"

namespace ms {
    // Initialize global namespace
    namespace obj {}
    namespace mtl {}
    namespace mvp {
        // MVP mode settings and data
    }
}

// Main entry point
int main() {
    try {
        ms::Setup::initialize();
        return 0;
    } catch (const std::exception& e) {
        // Handle initialization errors
        return 1;
    }
} 