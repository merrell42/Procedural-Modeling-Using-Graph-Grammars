#include "util.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <random>

namespace ms {

std::function<float()> Util::originalRandom = []() { return static_cast<float>(rand()) / RAND_MAX; };
int Util::randomCount = 0;

float Util::fixAngle(float angle) {
    while (angle > PI) {
        angle -= 2 * PI;
    }
    while (angle <= -PI) {
        angle += 2 * PI;
    }
    return angle;
}

float Util::angleDifference(float a, float b) {
    return fixAngle(a - b);
}

int Util::angleTurn(float prev, float next) {
    if (prev > PI - EPS) { prev -= 2 * PI; }
    if (next > PI - EPS) { next -= 2 * PI; }
    
    if (std::abs(prev - next) <= PI) {
        return 0;
    } else {
        if (prev >= 0 && next < 0) {
            return 1;
        } else if (next >= 0 && prev < 0) {
            return -1;
        }
    }
    return 0;
}

int Util::angleWedges(float prev, float next) {
    if (prev >= PI) { prev -= 2 * PI; }
    if (next >= PI) { next -= 2 * PI; }
    
    int sign = 0;
    if (prev >= 0 && next < 0) {
        sign = 1;
    } else if (next >= 0 && prev < 0) {
        sign = -1;
    }

    if (std::abs(prev - next) + 0.01f * sign > PI) {
        return sign;
    }
    return 0;
}

int Util::wedgeTurns(const std::vector<float>& angles) {
    int turns = 0;
    for (size_t i = 0; i < angles.size() - 1; i++) {
        turns += angleWedges(angles[i], angles[i + 1]);
    }
    return turns;
}

float Util::fixAngleWedges(float angle) {
    if (angle >= PI) { angle -= 2 * PI; }
    if (angle < -PI) { angle += 2 * PI; }
    return angle;
}

template<typename T>
void Util::remove(const T& element, std::vector<T>& array) {
    auto it = std::find(array.begin(), array.end(), element);
    if (it != array.end()) {
        array.erase(it);
    } else {
        throw std::runtime_error("Removing something that cannot be found.");
    }
}

template<typename T>
void Util::maybeRemove(const T& element, std::vector<T>& array) {
    auto it = std::find(array.begin(), array.end(), element);
    if (it != array.end()) {
        array.erase(it);
    }
}

template<typename T>
void Util::addToObject(std::vector<T>& obj, const std::string& prop, const T& value) {
    obj.push_back(value);
}

template<typename T>
void Util::addToArray(std::vector<T>& array, const std::vector<int>& indices, const T& value) {
    if (indices.empty()) return;
    
    int index = indices[0];
    if (array.size() <= static_cast<size_t>(index)) {
        array.resize(index + 1);
    }
    
    if (indices.size() > 1) {
        std::vector<int> remainingIndices(indices.begin() + 1, indices.end());
        addToArray(array[index], remainingIndices, value);
    } else {
        array[index].push_back(value);
    }
}

template<typename T>
void Util::union_(std::vector<T>& a, const std::vector<T>& b) {
    for (const auto& elem : b) {
        if (std::find(a.begin(), a.end(), elem) == a.end()) {
            a.push_back(elem);
        }
    }
}

int Util::consistentRandom(int n, int seed) {
    float x = (float)std::sin(seed) * (10000 + seed);
    float r = x - std::floor(x);
    return static_cast<int>(r * n);
}

/* float Util::random(int seed, int count) {
    float x = (float)std::sin(seed + count) * (10000 + seed);
    return x - std::floor(x);
} */

int Util::random(int count) {
    return rand() % count;
}

void Util::updateRandomMode() {
    // This would need to be adapted based on your global settings implementation
    // The original JS version uses ms.globalSettings
}

template<typename T>
std::vector<T> Util::removeDuplicates(const std::vector<T>& array) {
    std::vector<T> result;
    for (const auto& elem : array) {
        if (std::find(result.begin(), result.end(), elem) == result.end()) {
            result.push_back(elem);
        }
    }
    return result;
}

template<typename T>
bool Util::arraysEquivalent(const std::vector<T>& arrayA, const std::vector<T>& arrayB) {
    if (arrayA.size() != arrayB.size()) return false;
    
    for (const auto& elem : arrayA) {
        if (std::find(arrayB.begin(), arrayB.end(), elem) == arrayB.end()) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool Util::arraysEqual(const std::vector<T>& arrayA, const std::vector<T>& arrayB) {
    return arrayA == arrayB;
}

template<typename T>
const T& Util::last(const std::vector<T>& array) {
    if (array.empty()) {
        throw std::runtime_error("Cannot get last element of empty array");
    }
    return array.back();
}

float Util::clamp(float lower, float upper, float x) {
    return std::max(lower, std::min(upper, x));
}

std::vector<int> Util::sequence(int a, int b) {
    std::vector<int> result;
    result.reserve(b - a + 1);
    for (int i = a; i <= b; i++) {
        result.push_back(i);
    }
    return result;
}

template<typename T>
void Util::fastConcat(std::vector<T>& allData, const std::vector<T>& newData) {
    allData.insert(allData.end(), newData.begin(), newData.end());
}

int Util::maxDim(const Vec3& n) {
    std::array<std::pair<float, int>, 3> coords = {
        std::make_pair(std::abs(n.x), 0),
        std::make_pair(std::abs(n.y), 1),
        std::make_pair(std::abs(n.z), 2)
    };

    return std::max_element(coords.begin(), coords.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; }
    )->second;
}

int Util::randomDistribution(const std::vector<double>& probabilityMass) {
    std::vector<double> sums;
    double sum = 0.0;
    for (double pm : probabilityMass) {
        sum += pm;
        sums.push_back(sum);
    }
    if (sum == 0.0) {
        return -1;
    }
    double r = sum * (static_cast<double>(rand()) / RAND_MAX);
    int index = 0;
    while (index < sums.size() && r > sums[index]) {
        index++;
    }
    return (index < sums.size()) ? index : -1;
}

} // namespace ms 