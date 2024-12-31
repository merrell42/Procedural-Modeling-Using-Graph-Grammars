#include "m4.h"
#include <cmath>
#include <stdexcept>

namespace ms {

m4::Matrix4 m4::multiply(const Matrix4& a, const Matrix4& b, Matrix4* dst) {
    Matrix4 result = defaultMatrix();
    if (dst) result = *dst;

    const float b00 = b[0 * 4 + 0];
    const float b01 = b[0 * 4 + 1];
    const float b02 = b[0 * 4 + 2];
    const float b03 = b[0 * 4 + 3];
    const float b10 = b[1 * 4 + 0];
    const float b11 = b[1 * 4 + 1];
    const float b12 = b[1 * 4 + 2];
    const float b13 = b[1 * 4 + 3];
    const float b20 = b[2 * 4 + 0];
    const float b21 = b[2 * 4 + 1];
    const float b22 = b[2 * 4 + 2];
    const float b23 = b[2 * 4 + 3];
    const float b30 = b[3 * 4 + 0];
    const float b31 = b[3 * 4 + 1];
    const float b32 = b[3 * 4 + 2];
    const float b33 = b[3 * 4 + 3];
    const float a00 = a[0 * 4 + 0];
    const float a01 = a[0 * 4 + 1];
    const float a02 = a[0 * 4 + 2];
    const float a03 = a[0 * 4 + 3];
    const float a10 = a[1 * 4 + 0];
    const float a11 = a[1 * 4 + 1];
    const float a12 = a[1 * 4 + 2];
    const float a13 = a[1 * 4 + 3];
    const float a20 = a[2 * 4 + 0];
    const float a21 = a[2 * 4 + 1];
    const float a22 = a[2 * 4 + 2];
    const float a23 = a[2 * 4 + 3];
    const float a30 = a[3 * 4 + 0];
    const float a31 = a[3 * 4 + 1];
    const float a32 = a[3 * 4 + 2];
    const float a33 = a[3 * 4 + 3];

    result[0] = b00 * a00 + b01 * a10 + b02 * a20 + b03 * a30;
    result[1] = b00 * a01 + b01 * a11 + b02 * a21 + b03 * a31;
    result[2] = b00 * a02 + b01 * a12 + b02 * a22 + b03 * a32;
    result[3] = b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33;
    result[4] = b10 * a00 + b11 * a10 + b12 * a20 + b13 * a30;
    result[5] = b10 * a01 + b11 * a11 + b12 * a21 + b13 * a31;
    result[6] = b10 * a02 + b11 * a12 + b12 * a22 + b13 * a32;
    result[7] = b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33;
    result[8] = b20 * a00 + b21 * a10 + b22 * a20 + b23 * a30;
    result[9] = b20 * a01 + b21 * a11 + b22 * a21 + b23 * a31;
    result[10] = b20 * a02 + b21 * a12 + b22 * a22 + b23 * a32;
    result[11] = b20 * a03 + b21 * a13 + b22 * a23 + b23 * a33;
    result[12] = b30 * a00 + b31 * a10 + b32 * a20 + b33 * a30;
    result[13] = b30 * a01 + b31 * a11 + b32 * a21 + b33 * a31;
    result[14] = b30 * a02 + b31 * a12 + b32 * a22 + b33 * a32;
    result[15] = b30 * a03 + b31 * a13 + b32 * a23 + b33 * a33;

    return result;
}

m4::Vector3 m4::addVectors(const Vector3& a, const Vector3& b, Vector3* dst) {
    Vector3 result = defaultVector3();
    if (dst) result = *dst;
    
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
    
    return result;
}

m4::Vector3 m4::subtractVectors(const Vector3& a, const Vector3& b, Vector3* dst) {
    Vector3 result = defaultVector3();
    if (dst) result = *dst;
    
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
    
    return result;
}

m4::Vector3 m4::normalize(const Vector3& v, Vector3* dst) {
    Vector3 result = defaultVector3();
    if (dst) result = *dst;
    
    float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 0.00001f) {
        result[0] = v[0] / length;
        result[1] = v[1] / length;
        result[2] = v[2] / length;
    }
    
    return result;
}

m4::Matrix4 m4::defaultMatrix() {
    return {1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
}

m4::Vector3 m4::defaultVector3() {
    return {0, 0, 0};
}

// ... Additional implementations follow the same pattern
// Each method should be implemented following the original JavaScript logic
// but using C++ features and safety mechanisms

} // namespace ms 