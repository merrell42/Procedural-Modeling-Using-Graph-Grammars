#pragma once
#include <vector>
#include <functional>
#include <string>
#include "../shape/vec3.h"

namespace ms {

class Util {
public:
    // Angle operations
    static float fixAngle(float angle);
    static float angleDifference(float a, float b);
    static int angleTurn(float prev, float next);
    static int angleWedges(float prev, float next);
    static int wedgeTurns(const std::vector<float>& angles);
    static float fixAngleWedges(float angle);

    // Array operations
    template<typename T>
    static void remove(const T& element, std::vector<T>& array);
    
    template<typename T>
    static void maybeRemove(const T& element, std::vector<T>& array);
    
    template<typename T>
    static void addToObject(std::vector<T>& obj, const std::string& prop, const T& value);
    
    template<typename T>
    static void addToArray(std::vector<T>& array, const std::vector<int>& indices, const T& value);
    
    template<typename T>
    static void union_(std::vector<T>& a, const std::vector<T>& b);

    // Random operations
    static int consistentRandom(int n, int seed);
    static float random(int seed, int count);
    static void updateRandomMode();

    // Array utilities
    template<typename T>
    static std::vector<T> removeDuplicates(const std::vector<T>& array);
    
    template<typename T>
    static bool arraysEquivalent(const std::vector<T>& arrayA, const std::vector<T>& arrayB);
    
    template<typename T>
    static bool arraysEqual(const std::vector<T>& arrayA, const std::vector<T>& arrayB);
    
    template<typename T>
    static const T& last(const std::vector<T>& array);
    
    static float clamp(float lower, float upper, float x);
    static std::vector<int> sequence(int a, int b);
    
    template<typename T>
    static void fastConcat(std::vector<T>& allData, const std::vector<T>& newData);

    static int maxDim(const Vec3& n);

    static constexpr float EPS = 1e-5f;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float INF = 1e10;

private:
    static std::function<float()> originalRandom;
    static int randomCount;
};

// std::function<void()> noop = []() {};

} // namespace ms   