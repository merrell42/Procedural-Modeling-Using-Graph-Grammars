#pragma once
#include <vector>
#include <memory>
#include "../memory_counter.h"

using namespace std;

class Matrix {
public:
    Matrix(const Matrix& newMatrix);
    Matrix(const vector<vector<double>>& data);
    ~Matrix();
    static Matrix* zeros(int size0, int size1);

    const vector<vector<double>>& valueOf() const { return data; }
    const vector<int>& getSize() const { return size; }
    static Matrix* multiply(const Matrix* A, const Matrix* B);

    // For inverse and det the matrix is assumed to be a 3x3 matrix.
    static Matrix* inverse(const Matrix* A);
    static double det(const Matrix& A);
    static double det2x2(double a, double b, double c, double d);

private:
    vector<vector<double>> data;
    vector<int> size;
};

