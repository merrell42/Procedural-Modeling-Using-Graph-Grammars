#pragma once
#include <vector>
#include <functional>
#include <string>
#include "../shape/vec3.h"

namespace ms {

template <typename T> static
int indexOf(const std::vector<T>& vec, const T& value) {
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
        return std::distance(vec.begin(), it); // Return the index
    }
    return -1; // Return -1 if the value is not found
}

template <typename T> static
bool contains(const std::vector<T>& vec, const T& value) {
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

// Generate a random value. Like rand(), but the same every time.
double random();
void resetRandom(int seed);

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
    static void remove(std::vector<T>& array, const T& element) {
        auto it = std::find(array.begin(), array.end(), element);
        if (it != array.end()) {
            array.erase(it);
        }
    }
    
    template<typename T>
    static void maybeRemove(const T& element, std::vector<T>& array);
    
    template<typename T>
    static void addToObject(std::vector<T>& obj, const std::string& prop, const T& value);
    
    template<typename T>
    static void addToArray(std::vector<T>& array, const std::vector<int>& indices, const T& value);

    template<typename T>
    static void union_(std::vector<T>& a, const std::vector<T>& b) {
        for (const auto& elem : b) {
            if (std::find(a.begin(), a.end(), elem) == a.end()) {
                a.push_back(elem);
            }
        }
    }

    template <typename T>
    static int findIndex(const std::vector<T>& vec, const T& item) {
        // Use std::find to locate the item in the vector
        auto it = std::find(vec.begin(), vec.end(), item);

        // If the item is found, calculate and return the index
        if (it != vec.end()) {
            return std::distance(vec.begin(), it);
        }

        // If the item is not found, return -1 or any other indicator
        return -1; // Indicating that the item was not found
    }

    // Random operations
    static int randomInt(int count);
    static int randomDistribution(const std::vector<double>& probabilityMass);

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

    template <typename T>
    static const T& pick(const std::vector<T>& vec) {
        if (vec.empty()) {
            throw std::invalid_argument("Cannot pick an item from an empty vector.");
        }
        return vec[randomInt(vec.size())];
    }

    static double randomUniform(double lower, double upper);

    static constexpr float EPS = 1e-5f;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float INF = 1e10;

private:
    static std::function<float()> originalRandom;
    static int randomCount;
};

// std::function<void()> noop = []() {};

} // namespace ms   