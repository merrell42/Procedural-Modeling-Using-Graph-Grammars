#pragma once
#include <vector>
#include <memory>

using namespace std;

class Matrix {
public:
    Matrix(const vector<vector<double>>& data);
    ~Matrix() = default;
    static Matrix* zeros(int size0, int size1);

    const vector<vector<double>>& valueOf() const { return data; }
    const vector<int>& getSize() const { return size; }
    static Matrix* multiply(const Matrix* A, const Matrix* B);
    static Matrix* inverse(const Matrix* A);
    static double det(const Matrix& A);
    static double det2x2(double a, double b, double c, double d);

private:
    vector<vector<double>> data;
    vector<int> size;
};

