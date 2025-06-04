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
        return (int)std::distance(vec.begin(), it);
    }
    // Return -1 if not found
    return -1; 
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
    static double fixAngle(double angle);
    static double angleDifference(double a, double b);
    static int angleTurn(double prev, double next);
    static int angleWedges(double prev, double next);
    static int wedgeTurns(const std::vector<double>& angles);
    static double fixAngleWedges(double angle);

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
        auto it = std::find(vec.begin(), vec.end(), item);
        if (it != vec.end()) {
            return (int)std::distance(vec.begin(), it);
        }
        // Item not found.
        return -1;
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
    
    static double clamp(double lower, double upper, double x);
    static std::vector<int> sequence(int a, int b);
    
    template<typename T>
    static void fastConcat(std::vector<T>& allData, const std::vector<T>& newData);

    static int maxDim(const Vec3& n);

    template <typename T>
    static const T& pick(const std::vector<T>& vec) {
        if (vec.empty()) {
            throw std::invalid_argument("Cannot pick an item from an empty vector.");
        }
        return vec[randomInt((int)vec.size())];
    }

    static double randomUniform(double lower, double upper);

    static constexpr double EPS = 1e-5f;
    static constexpr double PI = 3.14159265358979323846f;
    static constexpr double INF = 1e10;

private:
    static std::function<double()> originalRandom;
    static int randomCount;
};

} // namespace ms   