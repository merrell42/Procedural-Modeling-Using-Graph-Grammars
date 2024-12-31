#pragma once
#include <string>
#include <vector>
#include <functional>
#include <map>

namespace ms {

class TestRunner {
public:
    static void run(const std::string& name = "");
    static void addTest(const std::string& name, std::function<void()> test);
    
    // Test assertion methods
    static void assert(bool condition, const std::string& message = "");
    static void assertEqual(const std::string& actual, const std::string& expected,
                          const std::string& message = "");
    static void assertEqual(int actual, int expected,
                          const std::string& message = "");
    static void assertEqual(float actual, float expected, float epsilon = 0.0001f,
                          const std::string& message = "");
    static void assertThrows(std::function<void()> fn,
                           const std::string& expectedError = "");

private:
    struct TestCase {
        std::string name;
        std::function<void()> test;
        bool passed;
        std::string error;
    };

    static std::map<std::string, TestCase> tests;
    static std::vector<std::string> failures;
    static int totalTests;
    static int passedTests;

    static void runTest(TestCase& test);
    static void printResults();
    static std::string formatTestName(const std::string& name);
};

} // namespace ms 