#pragma once
#include <vector>
#include <functional>
#include <string>
#include <unordered_set>
#include "../geometry/vec3.h"

// Find the index of an item in a vector.
template <typename T> static
int indexOf(const vector<T>& vec, const T& item) {
    auto it = find(vec.begin(), vec.end(), item);
    if (it != vec.end()) {
        return (int)distance(vec.begin(), it);
    }
    // Return -1 if not found.
    return -1; 
}

// Check if a vector contains an item.
template <typename T> static
bool contains(const vector<T>& vec, const T& item) {
    return find(vec.begin(), vec.end(), item) != vec.end();
}

// Generate a random number between 0 and 1. Like rand(), but it is deterministic.
// When we rerun the program, the same random numbers are generated.
double random();

// Reset the random number generator to a specific seed.
void resetRandom(int seed);

class Util {
public:
    // Remove an item from a vector.
    template<typename T>
    static inline void remove(vector<T>& array, const T& item) {
        array.erase(std::remove(array.begin(), array.end(), item), array.end());
    }

    // Find the union of two vectors.
    template<typename T>
    static void union_(vector<T>& a, const vector<T>& b) {
        unordered_set<T> existing(a.begin(), a.end());
        for (const auto& elem : b) {
            if (existing.insert(elem).second) {
                a.push_back(elem);
            }
        }
    }

    // Pick a random integer between 0 and count - 1.
    static int randomInt(int count);

    // Pick a random integer that is weighted by the probability mass.
    static int randomDistribution(const vector<double>& probabilityMass);

    // Find the dimension of the vector with the largest magnitude.
    static int maxDim(const Vec3& n);

    // Pick a random item from a vector.
    template <typename T>
    static const T& pick(const vector<T>& vec) {
        if (vec.empty()) {
            throw invalid_argument("Cannot pick an item from an empty vector.");
        }
        return vec[randomInt((int)vec.size())];
    }

    // Pick a random number between two values.
    static double randomUniform(double lower, double upper);
};
