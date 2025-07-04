#include "pch.h"
#include "range.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "util.h"
#include "minmax.h"

Range::Range()
    : low(0), high(0), tileLength(0) {}

Range::Range(double low_, double high_, double tilelen)
    : low(low_), high(high_), tileLength(tilelen) {}

void Range::intersect(const Range& rangeB) {
    low = max(low, rangeB.low);
    high = min(high, rangeB.high);
    tileLength = lcm(tileLength, rangeB.tileLength);
}

bool Range::isEmpty() const {
    return high < low;
}

double Range::sample() const {
    if (tileLength == 0) {
        return Util::randomUniform(low, high);
    } else {
        double l = ceil(low / tileLength);
        double h = floor(high / tileLength);
        return l + Util::randomUniform(0, h - l + 1);
    }
}

bool Range::isInside(double x) const {
    double m = ERROR_MARGIN;
    return (low - m <= x) && (x <= high + m);
}

Range Range::transform(double a, double b) const {
    if (a > 0) {
        return Range(a * low + b, a * high + b, a * tileLength);
    } else {
        return Range(a * high + b, a * low + b, a * tileLength);
    }
}

Range Range::transformCreate(double a, double b, const Range& rangeB) {
    if (abs(a) < ERROR_MARGIN) {
        if (rangeB.isInside(b)) {
            return Range(-INFINITY, INFINITY);
        } else {
            return Range(INFINITY, -INFINITY);  // Empty range
        }
    }
    return rangeB.transform(1.0f / a, -b / a);
}

double Range::gcd2(double x, double y) {
    return (y < ERROR_MARGIN) ? x : gcd2(y, fmod(x, y));
}

double Range::gcd(const vector<double>& arr) {
    return accumulate(arr.begin(), arr.end(), arr[0],
        [](double a, double b) { return gcd2(a, b); });
}

inline double Range::lcm(double a, double b) {
    if (abs(a) < ERROR_MARGIN) {
        return b;
    }
    if (abs(b) < ERROR_MARGIN) {
        return a;
    }
    return (a * b) / gcd2(a, b);
}

double Range::getLow() const { return low; }
double Range::getHigh() const { return high; }
double Range::getTileLength() const { return tileLength; }