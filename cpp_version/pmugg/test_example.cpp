#include "test_runner.h"
#include "vec2.h"

namespace ms {

void testVec2() {
    TestRunner::addTest("Vec2.constructor", []() {
        Vec2 v(1.0f, 2.0f);
        TestRunner::assertEqual(v.x, 1.0f);
        TestRunner::assertEqual(v.y, 2.0f);
    });

    TestRunner::addTest("Vec2.add", []() {
        Vec2 v1(1.0f, 2.0f);
        Vec2 v2(3.0f, 4.0f);
        Vec2 result = v1 + v2;
        TestRunner::assertEqual(result.x, 4.0f);
        TestRunner::assertEqual(result.y, 6.0f);
    });

    // Add more Vec2 tests...
}

void registerTests() {
    testVec2();
    // Register more test suites...
}

} // namespace ms

int main() {
    ms::registerTests();
    ms::TestRunner::run();
    return 0;
} 