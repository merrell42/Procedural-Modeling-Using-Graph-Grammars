#include "pch.h"
#include "range.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "util.h"
#include "minmax.h"

Range::Range()
    : data({ 0, 0 })
    , tileLength(0) {}

Range::Range(double low, double high, double tilelen)
    : data({low, high})
    , tileLength(tilelen) {}

Range Range::intersect(const Range& rangeB) const {
    return Range(
        max(data[0], rangeB.data[0]),
        min(data[1], rangeB.data[1]),
        lcm(tileLength, rangeB.tileLength)
    );
}

bool Range::isEmpty() const {
    return data[1] < data[0];
}

double Range::sample() const {
    if (tileLength == 0) {
        return Util::randomUniform(data[0], data[1]);
    } else {
        double low = ceil(data[0] / tileLength);
        double high = floor(data[1] / tileLength);
        return low + Util::randomUniform(0, high - low + 1);
    }
}

bool Range::isInside(double x) const {
    double m = ERROR_MARGIN;
    return (data[0] - m <= x) && (x <= data[1] + m);
}

Range Range::transform(double a, double b) const {
    if (a > 0) {
        return Range(a * data[0] + b, a * data[1] + b, a * tileLength);
    } else {
        return Range(a * data[1] + b, a * data[0] + b, a * tileLength);
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

void Range::print() const {
    cout << data[0] << ", " << data[1] << endl;
}

double Range::gcd2(double x, double y) {
    return (y < ERROR_MARGIN) ? x : gcd2(y, fmod(x, y));
}

double Range::gcd(const vector<double>& arr) {
    return accumulate(arr.begin(), arr.end(), arr[0],
        [](double a, double b) { return gcd2(a, b); });
}

double Range::lcm(double a, double b) {
    if (abs(a) < ERROR_MARGIN) {
        return b;
    }
    if (abs(b) < ERROR_MARGIN) {
        return a;
    }
    return (a * b) / gcd2(a, b);
}

const vector<double>& Range::getData() const {
    return data;
}

double Range::getTileLength() const {
    return tileLength;
}