#include "pch.h"
#include "matrix.h"
#include <stdexcept>
#include <cmath>

Matrix::Matrix(const vector<vector<double>>& d)
    : data(d) {
    size = {static_cast<int>(data.size()),
            static_cast<int>(data.empty() ? 0 : data[0].size())};
}

void Matrix::set(const vector<int>& index, double value) {
    data[index[0]][index[1]] = value;
}

double Matrix::get(const vector<int>& index) const {
    return data[index[0]][index[1]];
}

Matrix Matrix::subset(const vector<vector<int>>& indices,
                                        const Matrix* replacement) {
    const auto& indices0 = indices[0];
    const auto& indices1 = indices[1];

    if (replacement) {
        for (size_t i = 0; i < indices0.size(); i++) {
            for (size_t j = 0; j < indices1.size(); j++) {
                data[indices0[i]][indices1[j]] = replacement->data[i][j];
            }
        }
        return *this;
    }

    vector<vector<double>> result(indices0.size(),
                                          vector<double>(indices1.size()));
    for (size_t i = 0; i < indices0.size(); i++) {
        for (size_t j = 0; j < indices1.size(); j++) {
            result[i][j] = data[indices0[i]][indices1[j]];
        }
    }
    return Matrix(result);
}

Matrix& Matrix::concat(const Matrix& B) {
    size[1] += B.size[1];
    for (size_t i = 0; i < data.size(); i++) {
        data[i].insert(data[i].end(), B.data[i].begin(), B.data[i].end());
    }
    return *this;
}

Matrix Matrix::matrix(const vector<vector<double>>& data) {
    return Matrix(data);
}

Matrix* Matrix::zeros(int size0, int size1) {
    return new Matrix(vector<vector<double>>(size0,
                                                 vector<double>(size1, 0.0)));
}

Matrix* Matrix::add(const Matrix* A, const Matrix* B) {
    auto result = zeros(A->size[0], A->size[1]);
    for (int i = 0; i < A->size[0]; i++) {
        for (int j = 0; j < A->size[1]; j++) {
            result->data[i][j] = A->data[i][j] + B->data[i][j];
        }
    }
    return result;
}

Matrix* Matrix::subtract(const Matrix* A, const Matrix* B) {
    auto result = zeros(A->size[0], A->size[1]);
    for (int i = 0; i < A->size[0]; i++) {
        for (int j = 0; j < A->size[1]; j++) {
            result->data[i][j] = A->data[i][j] - B->data[i][j];
        }
    }
    return result;
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

Matrix* Matrix::multiply(const Matrix* A, double scalar) {
    auto result = zeros(A->size[0], A->size[1]);
    for (int i = 0; i < A->size[0]; i++) {
        for (int j = 0; j < A->size[1]; j++) {
            result->data[i][j] = A->data[i][j] * scalar;
        }
    }
    return result;
}

Matrix* Matrix::transpose(const Matrix* A) {
    auto result = zeros(A->size[1], A->size[0]);
    for (int i = 0; i < A->size[0]; i++) {
        for (int j = 0; j < A->size[1]; j++) {
            result->data[j][i] = A->data[i][j];
        }
    }
    return result;
}

double Matrix::det2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

double Matrix::det(const Matrix& A) {
    if (A.size[0] == 2 && A.size[1] == 2) {
        return det2x2(A.data[0][0], A.data[0][1],
                     A.data[1][0], A.data[1][1]);
    } else if (A.size[0] == 3 && A.size[1] == 3) {
        const auto& d = A.data;
        double a = d[0][0], b = d[0][1], c = d[0][2];
        double e = d[1][0], f = d[1][1], g = d[1][2];
        double h = d[2][0], i = d[2][1], j = d[2][2];
        return a*f*j + b*g*h + c*e*i - c*f*h - b*e*j - a*g*i;
    }
    throw runtime_error("det only implemented for 2x2 and 3x3");
}

Matrix* Matrix::inverse(const Matrix* A) {
    if (A->size[0] == 2 && A->size[1] == 2) {
        double det1 = 1.0 / det(*A);
        const auto& a = A->data;
        return new Matrix({{det1 * a[1][1], -det1 * a[0][1]},
                      {-det1 * a[1][0], det1 * a[0][0]}});
    } else if (A->size[0] == 3 && A->size[1] == 3) {
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
    throw runtime_error("inv only implemented for 2x2 and 3x3");
}

double Matrix::dot(const Matrix& A, const Matrix& B) {
    double sum = 0;
    for (int i = 0; i < A.size[0]; i++) {
        sum += A.data[i][0] * B.data[i][0];
    }
    return sum;
}

vector<int> Matrix::index(int index0, int index1) {
    return {index0, index1};
}

vector<int> Matrix::range(int lower, int upper) {
    vector<int> result;
    for (int i = lower; i < upper; i++) {
        result.push_back(i);
    }
    return result;
}

