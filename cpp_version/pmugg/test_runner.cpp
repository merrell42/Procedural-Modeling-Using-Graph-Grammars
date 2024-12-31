#include "test_runner.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace ms {

std::map<std::string, TestRunner::TestCase> TestRunner::tests;
std::vector<std::string> TestRunner::failures;
int TestRunner::totalTests = 0;
int TestRunner::passedTests = 0;

void TestRunner::run(const std::string& name) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (name.empty()) {
        // Run all tests
        std::cout << "\nRunning all tests...\n" << std::endl;
        for (auto& [testName, test] : tests) {
            runTest(test);
        }
    } else {
        // Run specific test
        auto it = tests.find(name);
        if (it != tests.end()) {
            std::cout << "\nRunning test: " << name << "\n" << std::endl;
            runTest(it->second);
        } else {
            std::cout << "Test '" << name << "' not found." << std::endl;
            return;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    printResults();
    std::cout << "Time: " << duration.count() << "ms" << std::endl;
}

void TestRunner::addTest(const std::string& name, std::function<void()> test) {
    tests[name] = TestCase{name, test, false, ""};
}

void TestRunner::assert(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message.empty() ? "Assertion failed" : message);
    }
}

void TestRunner::assertEqual(const std::string& actual, const std::string& expected,
                           const std::string& message) {
    if (actual != expected) {
        std::string error = "Expected '" + expected + "' but got '" + actual + "'";
        if (!message.empty()) {
            error = message + ": " + error;
        }
        throw std::runtime_error(error);
    }
}

void TestRunner::assertEqual(int actual, int expected, const std::string& message) {
    if (actual != expected) {
        std::string error = "Expected " + std::to_string(expected) + 
                           " but got " + std::to_string(actual);
        if (!message.empty()) {
            error = message + ": " + error;
        }
        throw std::runtime_error(error);
    }
}

void TestRunner::assertEqual(float actual, float expected, float epsilon,
                           const std::string& message) {
    if (std::abs(actual - expected) > epsilon) {
        std::string error = "Expected " + std::to_string(expected) + 
                           " but got " + std::to_string(actual);
        if (!message.empty()) {
            error = message + ": " + error;
        }
        throw std::runtime_error(error);
    }
}

void TestRunner::assertThrows(std::function<void()> fn,
                            const std::string& expectedError) {
    try {
        fn();
        throw std::runtime_error("Expected an error but none was thrown");
    } catch (const std::exception& e) {
        if (!expectedError.empty() && e.what() != expectedError) {
            throw std::runtime_error("Expected error '" + expectedError + 
                                   "' but got '" + e.what() + "'");
        }
    }
}

void TestRunner::runTest(TestCase& test) {
    totalTests++;
    
    try {
        test.test();
        test.passed = true;
        passedTests++;
        std::cout << "\x1B[32m✓\x1B[0m " << formatTestName(test.name) << std::endl;
    } catch (const std::exception& e) {
        test.passed = false;
        test.error = e.what();
        failures.push_back(test.name + ": " + e.what());
        std::cout << "\x1B[31m✗\x1B[0m " << formatTestName(test.name) << std::endl;
        std::cout << "  " << e.what() << std::endl;
    }
}

void TestRunner::printResults() {
    std::cout << "\nResults:\n";
    std::cout << "  Total tests: " << totalTests << std::endl;
    std::cout << "  Passed: " << passedTests << std::endl;
    std::cout << "  Failed: " << (totalTests - passedTests) << std::endl;
    
    if (!failures.empty()) {
        std::cout << "\nFailures:\n";
        for (const auto& failure : failures) {
            std::cout << "  " << failure << std::endl;
        }
    }
}

std::string TestRunner::formatTestName(const std::string& name) {
    return name;
}

} // namespace ms 