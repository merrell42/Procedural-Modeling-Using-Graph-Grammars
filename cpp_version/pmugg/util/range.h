#pragma once
#include <vector>

namespace ms {

class Range {
public:
    Range();
    Range(float low, float high, float tileLength = 0);
    ~Range() = default;

    const std::vector<float>& getData() const { return data; }
    float getTileLength() const { return tileLength; }

    Range intersect(const Range& rangeB) const;
    bool isEmpty() const;
    float sample() const;
    bool isInside(float x) const;
    Range transform(float a, float b) const;
    void print() const;

    static Range transformCreate(float a, float b, const Range& rangeB);
    static float gcd(const std::vector<float>& arr);
    static float lcm(float a, float b);

    static constexpr float ERROR_MARGIN = 1e-5f;

private:
    std::vector<float> data;  // [low, high]
    float tileLength;

    static float gcd2(float x, float y);
};

} // namespace ms 