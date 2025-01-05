#pragma once
#include <vector>
#include <memory>

namespace ms {

// class FastMath {
// public:
class Matrix {
public:
    Matrix(const std::vector<std::vector<double>>& data);
    ~Matrix() = default;

    // Core accessors
    const std::vector<std::vector<double>>& valueOf() const { return data; }
    const std::vector<int>& getSize() const { return size; }

    // Matrix operations
    void set(const std::vector<int>& index, double value);
    double get(const std::vector<int>& index) const;
    Matrix subset(const std::vector<std::vector<int>>& indices, const Matrix* replacement = nullptr);
    Matrix& concat(const Matrix& B);

    // Matrix creation
    static Matrix matrix(const std::vector<std::vector<double>>& data);
    static Matrix* zeros(int size0, int size1);

    // Matrix operations
    static Matrix* add(const Matrix* A, const Matrix* B);
    static Matrix* subtract(const Matrix* A, const Matrix* B);
    static Matrix* multiply(const Matrix* A, const Matrix* B);
    static Matrix* multiply(const Matrix* A, double scalar);
    static Matrix* transpose(const Matrix* A);
    static Matrix* inverse(const Matrix* A);
    static double dot(const Matrix& A, const Matrix& B);
    static double det(const Matrix& A);

    // Helper functions
    static double det2x2(double a, double b, double c, double d);
    static std::vector<int> index(int index0, int index1);
    static std::vector<int> range(int lower, int upper);

private:
    std::vector<std::vector<double>> data;
    std::vector<int> size;
};

//     private:
//     static constexpr double EPSILON = 1e-10;
// };

} // namespace ms 