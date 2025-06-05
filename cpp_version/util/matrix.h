#pragma once
#include <vector>
#include <memory>

using namespace std;

namespace ms {

class Matrix {
public:
    Matrix(const vector<vector<double>>& data);
    ~Matrix() = default;

    // Matrix creation
    static Matrix matrix(const vector<vector<double>>& data);
    static Matrix* zeros(int size0, int size1);

    // Core accessors
    const vector<vector<double>>& valueOf() const { return data; }
    const vector<int>& getSize() const { return size; }

    // Matrix operations
    void set(const vector<int>& index, double value);
    double get(const vector<int>& index) const;
    Matrix subset(const vector<vector<int>>& indices, const Matrix* replacement = nullptr);
    Matrix& concat(const Matrix& B);

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
    static vector<int> index(int index0, int index1);
    static vector<int> range(int lower, int upper);

private:
    vector<vector<double>> data;
    vector<int> size;
};

} // namespace ms 