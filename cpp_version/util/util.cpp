#include "pch.h"
#include "util.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <iostream>
#include "minmax.h"

int randomCount = 0;
int randomSeed = 42;

double random() {
    double x = (double)sin(randomSeed + randomCount) * (10000 + randomSeed);
    randomCount++;
    return x - floor(x);
}

void resetRandom(int seed) {
    randomSeed = seed;
    randomCount = 0;
}

double Util::fixAngle(double angle) {
    while (angle > PI) {
        angle -= 2 * PI;
    }
    while (angle <= -PI) {
        angle += 2 * PI;
    }
    return angle;
}

double Util::angleDifference(double a, double b) {
    return fixAngle(a - b);
}

int Util::angleTurn(double prev, double next) {
    if (prev > PI - EPS) { prev -= 2 * PI; }
    if (next > PI - EPS) { next -= 2 * PI; }
    
    if (abs(prev - next) <= PI) {
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

int Util::angleWedges(double prev, double next) {
    if (prev >= PI) { prev -= 2 * PI; }
    if (next >= PI) { next -= 2 * PI; }
    
    int sign = 0;
    if (prev >= 0 && next < 0) {
        sign = 1;
    } else if (next >= 0 && prev < 0) {
        sign = -1;
    }

    if (abs(prev - next) + 0.01f * sign > PI) {
        return sign;
    }
    return 0;
}

int Util::wedgeTurns(const vector<double>& angles) {
    int turns = 0;
    for (size_t i = 0; i < angles.size() - 1; i++) {
        turns += angleWedges(angles[i], angles[i + 1]);
    }
    return turns;
}

double Util::fixAngleWedges(double angle) {
    if (angle >= PI) { angle -= 2 * PI; }
    if (angle < -PI) { angle += 2 * PI; }
    return angle;
}

template<typename T>
void Util::maybeRemove(const T& element, vector<T>& array) {
    auto it = find(array.begin(), array.end(), element);
    if (it != array.end()) {
        array.erase(it);
    }
}

template<typename T>
void Util::addToObject(vector<T>& obj, const string& prop, const T& value) {
    obj.push_back(value);
}

template<typename T>
void Util::addToArray(vector<T>& array, const vector<int>& indices, const T& value) {
    if (indices.empty()) return;
    
    int index = indices[0];
    if (array.size() <= static_cast<size_t>(index)) {
        array.resize(index + 1);
    }
    
    if (indices.size() > 1) {
        vector<int> remainingIndices(indices.begin() + 1, indices.end());
        addToArray(array[index], remainingIndices, value);
    } else {
        array[index].push_back(value);
    }
}

int Util::randomInt(int count) {
    return static_cast<int>(random() * count);
}

template<typename T>
vector<T> Util::removeDuplicates(const vector<T>& array) {
    vector<T> result;
    for (const auto& elem : array) {
        if (find(result.begin(), result.end(), elem) == result.end()) {
            result.push_back(elem);
        }
    }
    return result;
}

template<typename T>
bool Util::arraysEquivalent(const vector<T>& arrayA, const vector<T>& arrayB) {
    if (arrayA.size() != arrayB.size()) return false;
    
    for (const auto& elem : arrayA) {
        if (find(arrayB.begin(), arrayB.end(), elem) == arrayB.end()) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool Util::arraysEqual(const vector<T>& arrayA, const vector<T>& arrayB) {
    return arrayA == arrayB;
}

template<typename T>
const T& Util::last(const vector<T>& array) {
    if (array.empty()) {
        throw runtime_error("Cannot get last element of empty array");
    }
    return array.back();
}

double Util::clamp(double lower, double upper, double x) {
    return max(lower, min(upper, x));
}

vector<int> Util::sequence(int a, int b) {
    vector<int> result;
    result.reserve(b - a + 1);
    for (int i = a; i <= b; i++) {
        result.push_back(i);
    }
    return result;
}

template<typename T>
void Util::fastConcat(vector<T>& allData, const vector<T>& newData) {
    allData.insert(allData.end(), newData.begin(), newData.end());
}

int Util::maxDim(const Vec3& n) {
    array<pair<double, int>, 3> coords = {
        make_pair(abs(n.getX()), 0),
        make_pair(abs(n.getY()), 1),
        make_pair(abs(n.getZ()), 2)
    };

    return max_element(coords.begin(), coords.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; }
    )->second;
}

int Util::randomDistribution(const vector<double>& probabilityMass) {
    vector<double> sums;
    double sum = 0.0;
    for (double pm : probabilityMass) {
        sum += pm;
        sums.push_back(sum);
    }
    if (sum == 0.0) {
        return -1;
    }
    double r = sum * random();
    int index = 0;
    while (index < sums.size() && r > sums[index]) {
        index++;
    }
    return (index < sums.size()) ? index : -1;
}

// Function definition
double Util::randomUniform(double lower, double upper) {
    if (lower > upper) {
        throw invalid_argument("Lower bound is greater than upper bound.");
    }
    double s = random();
    return s * (upper - lower) + lower;
}

