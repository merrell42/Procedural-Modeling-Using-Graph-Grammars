#include "test_runner.h"
#include "test_scenario.h"

namespace ms {

void testTestScenario() {
    TestRunner::addTest("TestScenario.createShapes", []() {
        TestScenario scenario;
        
        // Test square creation
        auto square = TestScenario::createSquare(2.0f);
        TestRunner::assertEqual(square.size(), size_t(4));
        TestRunner::assertEqual(square[0].x, -1.0f);
        TestRunner::assertEqual(square[0].y, -1.0f);
        
        // Test triangle creation
        auto triangle = TestScenario::createTriangle(2.0f);
        TestRunner::assertEqual(triangle.size(), size_t(3));
        
        // Test circle creation
        auto circle = TestScenario::createCircle(1.0f, 8);
        TestRunner::assertEqual(circle.size(), size_t(8));
    });

    TestRunner::addTest("TestScenario.addTestObject", []() {
        TestScenario scenario;
        scenario.addTestObject();
        // Add assertions to verify the test object was created correctly
    });

    // Add more tests...
}

void registerTests() {
    testTestScenario();
}

} // namespace ms

int main() {
    ms::registerTests();
    ms::TestRunner::run();
    return 0;
} 