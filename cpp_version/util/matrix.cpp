#include "pch.h"
#include "matrix.h"
#include <stdexcept>
#include <cmath>

Matrix::Matrix(const Matrix& newMatrix)
    : data(newMatrix.data), size(newMatrix.size) {
    MemoryCounter::creation("Matrix");
}

Matrix::Matrix(const vector<vector<double>>& d)
    : data(d) {
    MemoryCounter::creation("Matrix");
    size = {static_cast<int>(data.size()),
            static_cast<int>(data.empty() ? 0 : data[0].size())};
}

Matrix::~Matrix() {
    MemoryCounter::destruction("Matrix");
}

Matrix* Matrix::zeros(int size0, int size1) {
    return new Matrix(vector<vector<double>>(size0, vector<double>(size1, 0.0)));
}

Matrix* Matrix::multiply(const Matrix* A, const Matrix* B) {
    auto result = zeros(A->size[0], B->size[1]);
    for (int i = 0; i < A->size[0]; i++) {
        for (int j = 0; j < B->size[1]; j++) {
            double sum = 0;
            for (int k = 0; k < A->size[1]; k++) {
                sum += A->data[i][k] * B->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }
    return result;
}

double Matrix::det2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

double Matrix::det(const Matrix& A) {
    const auto& d = A.data;
    double a = d[0][0], b = d[0][1], c = d[0][2];
    double e = d[1][0], f = d[1][1], g = d[1][2];
    double h = d[2][0], i = d[2][1], j = d[2][2];
    return a*f*j + b*g*h + c*e*i - c*f*h - b*e*j - a*g*i;
}

Matrix* Matrix::inverse(const Matrix* A) {
    double det1 = 1.0 / det(*A);
    const auto& d = A->data;
    vector<vector<double>> result(3, vector<double>(3));
        
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int s = (j + 1) % 3;
            int t = (j + 2) % 3;
            int u = (i + 1) % 3;
            int v = (i + 2) % 3;
            double a = d[s][u];
            double b = d[t][u];
            double c = d[s][v];
            double e = d[t][v];
            result[i][j] = det1 * det2x2(a, b, c, e);
        }
    }
    return new Matrix(result);
}
