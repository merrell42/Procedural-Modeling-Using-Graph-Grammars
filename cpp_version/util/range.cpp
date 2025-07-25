#include "pch.h"
#include "range.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "util.h"
#ifdef PMUGG_CMDLINE
#include "minmax.h"
#endif

Range::Range() : low(0), high(0), tileLength(0) {}

Range::Range(double low_, double high_, double tilelen)
    : low(low_), high(high_), tileLength(tilelen) {}

// Intersect two ranges. If they have tile lengths, compute the least common multiple.
void Range::intersect(const Range& rangeB) {
    low = max(low, rangeB.low);
    high = min(high, rangeB.high);
    tileLength = lcm(tileLength, rangeB.tileLength);
}

bool Range::isEmpty() const {
    return high < low;
}

// Sample a random value from the range.
double Range::sample() const {
    if (tileLength == 0) {
        return Util::randomUniform(low, high);
    } else {
        double l = ceil(low / tileLength);
        double h = floor(high / tileLength);
        return l + Util::randomUniform(0, h - l + 1);
    }
}

// Check if a value is inside the range.
bool Range::isInside(double x) const {
    double m = ERROR_MARGIN;
    return (low - m <= x) && (x <= high + m);
}

// Transform the range by a linear function.
Range Range::transform(double m, double b) const {
    if (m > 0) {
        return Range(m * low + b, m * high + b, m * tileLength);
    } else {
        return Range(m * high + b, m * low + b, m * tileLength);
    }
}

// Transform a range by a linear function.
Range Range::transformCreate(double m, double b, const Range& rangeB) {
    // If the slope is 0, return an infinite range if the value b is acceptable
    // and an empty range if it is not.
    if (abs(m) < ERROR_MARGIN) {
        if (rangeB.isInside(b)) {
            return Range(-INFINITY, INFINITY);
        } else {
            // Empty range.
            return Range(INFINITY, -INFINITY);
        }
    }
    // Otherwise, transform the range by the inverse of the slope.
    return rangeB.transform(1.0 / m, -b / m);
}

// Find the greatest common divisor of two numbers.
double Range::gcd2(double x, double y) {
    return (y < ERROR_MARGIN) ? x : gcd2(y, fmod(x, y));
}

// Find the greatest common divisor of an array of numbers.
double Range::gcd(const vector<double>& arr) {
    return accumulate(arr.begin(), arr.end(), arr[0],
        [](double a, double b) { return gcd2(a, b); });
}

// Find the least common multiple of two numbers.
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