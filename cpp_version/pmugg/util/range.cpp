#include "pch.h"
#include "range.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "util.h"
#include "minmax.h"

namespace ms {

Range::Range()
    : data({ 0, 0 })
    , tileLength(0) {}

Range::Range(float low, float high, float tilelen)
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

float Range::sample() const {
    if (tileLength == 0) {
        return Util::randomUniform(data[0], data[1]);
    } else {
        float low = std::ceil(data[0] / tileLength);
        float high = std::floor(data[1] / tileLength);
        return low + Util::randomUniform(0, high - low + 1);
    }
}

bool Range::isInside(float x) const {
    float m = ERROR_MARGIN;
    return (data[0] - m <= x) && (x <= data[1] + m);
}

Range Range::transform(float a, float b) const {
    if (a > 0) {
        return Range(a * data[0] + b, a * data[1] + b, a * tileLength);
    } else {
        return Range(a * data[1] + b, a * data[0] + b, a * tileLength);
    }
}

Range Range::transformCreate(float a, float b, const Range& rangeB) {
    if (std::abs(a) < ERROR_MARGIN) {
        if (rangeB.isInside(b)) {
            return Range(-INFINITY, INFINITY);
        } else {
            return Range(INFINITY, -INFINITY);  // Empty range
        }
    }
    return rangeB.transform(1.0f / a, -b / a);
}

void Range::print() const {
    std::cout << data[0] << ", " << data[1] << std::endl;
}

float Range::gcd2(float x, float y) {
    return (y < ERROR_MARGIN) ? x : gcd2(y, std::fmod(x, y));
}

float Range::gcd(const std::vector<float>& arr) {
    return std::accumulate(arr.begin(), arr.end(), arr[0],
        [](float a, float b) { return gcd2(a, b); });
}

float Range::lcm(float a, float b) {
    if (std::abs(a) < ERROR_MARGIN) {
        return b;
    }
    if (std::abs(b) < ERROR_MARGIN) {
        return a;
    }
    return (a * b) / gcd2(a, b);
}

} // namespace ms 