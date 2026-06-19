#include "pch.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <cmath>
#ifdef _CONSOLE
#include "minmax.h"
#endif
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>

static int randomCount = 0;
static int randomSeed = 42;

double randomValue() {
    double x = (double)sin(randomSeed + randomCount) * (10000 + randomSeed);
    randomCount++;
    return x - floor(x);
}

void resetRandom(int seed) {
    randomSeed = seed;
    randomCount = 0;
}

int Util::randomInt(int count) {
    return static_cast<int>(randomValue() * count);
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
    double r = sum * randomValue();
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
    double s = randomValue();
    return s * (upper - lower) + lower;
}

double Util::fixAngle(double angle) {
    while (angle > M_PI) {
        angle -= 2 * M_PI;
    }
    while (angle <= -M_PI) {
        angle += 2 * M_PI;
    }
    return angle;
}

int Util::angleWedges(double prev, double next) {
    if (prev >= M_PI) { prev -= 2 * M_PI; }
    if (next >= M_PI) { next -= 2 * M_PI; }

    int sign = 0;
    if (prev >= 0 && next < 0) {
        sign = 1;
    } else if (next >= 0 && prev < 0) {
        sign = -1;
    }

    // The 0.01 sign makes positive turns more common when
    // angles are in opposite directions.
    if (abs(prev - next) + 0.01 * sign > M_PI) {
        return sign;
    }
    return 0;
}

int Util::wedgeTurns(const vector<double>& angles) {
    int turns = 0;
    for (size_t i = 0; i + 1 < angles.size(); i++) {
        turns += angleWedges(angles[i], angles[i + 1]);
    }
    return turns;
}

