#pragma once
#include <vector>
#include <functional>
#include <string>
#include "../geometry/vec3.h"

template <typename T> static
int indexOf(const vector<T>& vec, const T& value) {
    auto it = find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
        return (int)distance(vec.begin(), it);
    }
    // Return -1 if not found
    return -1; 
}

template <typename T> static
bool contains(const vector<T>& vec, const T& value) {
    return find(vec.begin(), vec.end(), value) != vec.end();
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
    static int wedgeTurns(const vector<double>& angles);
    static double fixAngleWedges(double angle);

    // Array operations
    template<typename T>
    static void remove(vector<T>& array, const T& element) {
        auto it = find(array.begin(), array.end(), element);
        if (it != array.end()) {
            array.erase(it);
        }
    }
    
    template<typename T>
    static void maybeRemove(const T& element, vector<T>& array);
    
    template<typename T>
    static void addToObject(vector<T>& obj, const string& prop, const T& value);
    
    template<typename T>
    static void addToArray(vector<T>& array, const vector<int>& indices, const T& value);

    template<typename T>
    static void union_(vector<T>& a, const vector<T>& b) {
        for (const auto& elem : b) {
            if (find(a.begin(), a.end(), elem) == a.end()) {
                a.push_back(elem);
            }
        }
    }

    template <typename T>
    static int findIndex(const vector<T>& vec, const T& item) {
        auto it = find(vec.begin(), vec.end(), item);
        if (it != vec.end()) {
            return (int)distance(vec.begin(), it);
        }
        // Item not found.
        return -1;
    }

    // Random operations
    static int randomInt(int count);
    static int randomDistribution(const vector<double>& probabilityMass);

    // Array utilities
    template<typename T>
    static vector<T> removeDuplicates(const vector<T>& array);
    
    template<typename T>
    static bool arraysEquivalent(const vector<T>& arrayA, const vector<T>& arrayB);
    
    template<typename T>
    static bool arraysEqual(const vector<T>& arrayA, const vector<T>& arrayB);
    
    template<typename T>
    static const T& last(const vector<T>& array);
    
    static double clamp(double lower, double upper, double x);
    static vector<int> sequence(int a, int b);
    
    template<typename T>
    static void fastConcat(vector<T>& allData, const vector<T>& newData);

    static int maxDim(const Vec3& n);

    template <typename T>
    static const T& pick(const vector<T>& vec) {
        if (vec.empty()) {
            throw invalid_argument("Cannot pick an item from an empty vector.");
        }
        return vec[randomInt((int)vec.size())];
    }

    static double randomUniform(double lower, double upper);

    static constexpr double EPS = 1e-5f;
    static constexpr double PI = 3.14159265358979323846f;
    static constexpr double INF = 1e10;

private:
    static function<double()> originalRandom;
    static int randomCount;
};
