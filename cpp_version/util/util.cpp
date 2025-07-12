#include "pch.h"
#include "util.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <iostream>
#include "minmax.h"

static int randomCount = 0;
static int randomSeed = 42;

double random() {
    double x = (double)sin(randomSeed + randomCount) * (10000 + randomSeed);
    randomCount++;
    return x - floor(x);
}

void resetRandom(int seed) {
    randomSeed = seed;
    randomCount = 0;
}

int Util::randomInt(int count) {
    return static_cast<int>(random() * count);
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

