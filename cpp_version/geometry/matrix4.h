#pragma once

class Vec3;

class Matrix4 {
public:
    // Column-major 4x4 matrix. Element at (row, col) is m[col * 4 + row].
    float m[16];

    Matrix4();

    static Matrix4 identity();
    static Matrix4 translation(float x, float y, float z);
    static Matrix4 translation(const Vec3& position);
    static Matrix4 rotation(float x, float y, float z, float angleRadians);
    static Matrix4 scale(float x, float y, float z);

    Matrix4 operator*(const Matrix4& other) const;
    void copyTo(float out[16]) const;
};
