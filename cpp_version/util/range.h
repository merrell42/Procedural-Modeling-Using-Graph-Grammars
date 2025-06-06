#pragma once
#include <vector>

using namespace std;

class Range {
public:
    Range();
    Range(double low, double high, double tileLength = 0);
    ~Range() = default;

    const vector<double>& getData() const { return data; }
    double getTileLength() const { return tileLength; }

    Range intersect(const Range& rangeB) const;
    bool isEmpty() const;
    double sample() const;
    bool isInside(double x) const;
    Range transform(double a, double b) const;
    void print() const;

    static Range transformCreate(double a, double b, const Range& rangeB);
    static double gcd(const vector<double>& arr);
    static double lcm(double a, double b);

    static constexpr double ERROR_MARGIN = 1e-5f;

private:
    vector<double> data;  // [low, high]
    double tileLength;

    static double gcd2(double x, double y);
};

