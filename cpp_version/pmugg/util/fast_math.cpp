#include "fast_math.h"
#include <stdexcept>
#include <cmath>

namespace ms {

FastMath::Matrix::Matrix(const std::vector<std::vector<double>>& d)
    : data(d) {
    size = {static_cast<int>(data.size()),
            static_cast<int>(data.empty() ? 0 : data[0].size())};
}

void FastMath::Matrix::set(const std::vector<int>& index, double value) {
    data[index[0]][index[1]] = value;
}

double FastMath::Matrix::get(const std::vector<int>& index) const {
    return data[index[0]][index[1]];
}

FastMath::Matrix FastMath::Matrix::subset(const std::vector<std::vector<int>>& indices,
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

    std::vector<std::vector<double>> result(indices0.size(),
                                          std::vector<double>(indices1.size()));
    for (size_t i = 0; i < indices0.size(); i++) {
        for (size_t j = 0; j < indices1.size(); j++) {
            result[i][j] = data[indices0[i]][indices1[j]];
        }
    }
    return Matrix(result);
}

FastMath::Matrix& FastMath::Matrix::concat(const Matrix& B) {
    size[1] += B.size[1];
    for (size_t i = 0; i < data.size(); i++) {
        data[i].insert(data[i].end(), B.data[i].begin(), B.data[i].end());
    }
    return *this;
}

FastMath::Matrix FastMath::matrix(const std::vector<std::vector<double>>& data) {
    return Matrix(data);
}

FastMath::Matrix FastMath::zeros(int size0, int size1) {
    return Matrix(std::vector<std::vector<double>>(size0,
                                                 std::vector<double>(size1, 0.0)));
}

FastMath::Matrix FastMath::add(const Matrix& A, const Matrix& B) {
    auto result = zeros(A.size[0], A.size[1]);
    for (int i = 0; i < A.size[0]; i++) {
        for (int j = 0; j < A.size[1]; j++) {
            result.data[i][j] = A.data[i][j] + B.data[i][j];
        }
    }
    return result;
}

FastMath::Matrix FastMath::subtract(const Matrix& A, const Matrix& B) {
    auto result = zeros(A.size[0], A.size[1]);
    for (int i = 0; i < A.size[0]; i++) {
        for (int j = 0; j < A.size[1]; j++) {
            result.data[i][j] = A.data[i][j] - B.data[i][j];
        }
    }
    return result;
}

FastMath::Matrix FastMath::multiply(const Matrix& A, const Matrix& B) {
    auto result = zeros(A.size[0], B.size[1]);
    for (int i = 0; i < A.size[0]; i++) {
        for (int j = 0; j < B.size[1]; j++) {
            double sum = 0;
            for (int k = 0; k < A.size[1]; k++) {
                sum += A.data[i][k] * B.data[k][j];
            }
            result.data[i][j] = sum;
        }
    }
    return result;
}

FastMath::Matrix FastMath::multiply(const Matrix& A, double scalar) {
    auto result = zeros(A.size[0], A.size[1]);
    for (int i = 0; i < A.size[0]; i++) {
        for (int j = 0; j < A.size[1]; j++) {
            result.data[i][j] = A.data[i][j] * scalar;
        }
    }
    return result;
}

FastMath::Matrix FastMath::transpose(const Matrix& A) {
    auto result = zeros(A.size[1], A.size[0]);
    for (int i = 0; i < A.size[0]; i++) {
        for (int j = 0; j < A.size[1]; j++) {
            result.data[j][i] = A.data[i][j];
        }
    }
    return result;
}

double FastMath::det2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

double FastMath::det(const Matrix& A) {
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
    throw std::runtime_error("det only implemented for 2x2 and 3x3");
}

FastMath::Matrix FastMath::inv(const Matrix& A) {
    if (A.size[0] == 2 && A.size[1] == 2) {
        double det1 = 1.0 / det(A);
        const auto& a = A.data;
        return matrix({{det1 * a[1][1], -det1 * a[0][1]},
                      {-det1 * a[1][0], det1 * a[0][0]}});
    } else if (A.size[0] == 3 && A.size[1] == 3) {
        double det1 = 1.0 / det(A);
        const auto& d = A.data;
        std::vector<std::vector<double>> result(3, std::vector<double>(3));
        
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
        return matrix(result);
    }
    throw std::runtime_error("inv only implemented for 2x2 and 3x3");
}

double FastMath::dot(const Matrix& A, const Matrix& B) {
    double sum = 0;
    for (int i = 0; i < A.size[0]; i++) {
        sum += A.data[i][0] * B.data[i][0];
    }
    return sum;
}

std::vector<int> FastMath::index(int index0, int index1) {
    return {index0, index1};
}

std::vector<int> FastMath::range(int lower, int upper) {
    std::vector<int> result;
    for (int i = lower; i < upper; i++) {
        result.push_back(i);
    }
    return result;
}

void FastMath::test() {
    // Test matrix operations
    Matrix A({{0.5111, -0.2797}, {0.3514, 0.7387}});
    Matrix B({{0.9243, 0.8126}, {0.2027, 0.4439}});

    auto sum = add(A, B);
    auto diff = subtract(A, B);
    auto prod = multiply(A, B);
    auto invA = inv(A);

    // Add verification code as needed
}

} // namespace ms 