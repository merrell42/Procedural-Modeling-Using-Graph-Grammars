#include "pch.h"
#include "matrix4.h"
#include "vec3.h"

Matrix4::Matrix4() {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

Matrix4 Matrix4::identity() {
    return Matrix4();
}

Matrix4 Matrix4::translation(float x, float y, float z) {
    Matrix4 result;
    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;
    return result;
}

Matrix4 Matrix4::translation(const Vec3& position) {
    return translation(
        (float)position.getX(),
        (float)position.getY(),
        (float)position.getZ()
    );
}

Matrix4 Matrix4::rotation(float x, float y, float z, float angleRadians) {
    float length = sqrtf(x * x + y * y + z * z);
    if (length == 0.0f) {
        return Matrix4::identity();
    }
    x /= length;
    y /= length;
    z /= length;

    float c = cosf(angleRadians);
    float s = sinf(angleRadians);
    float t = 1.0f - c;

    Matrix4 result;
    result.m[0] = t * x * x + c;
    result.m[1] = t * x * y + s * z;
    result.m[2] = t * x * z - s * y;

    result.m[4] = t * x * y - s * z;
    result.m[5] = t * y * y + c;
    result.m[6] = t * y * z + s * x;

    result.m[8] = t * x * z + s * y;
    result.m[9] = t * y * z - s * x;
    result.m[10] = t * z * z + c;
    return result;
}

Matrix4 Matrix4::scale(float x, float y, float z) {
    Matrix4 result;
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 result;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += m[k * 4 + row] * other.m[col * 4 + k];
            }
            result.m[col * 4 + row] = sum;
        }
    }
    return result;
}

void Matrix4::copyTo(float out[16]) const {
    for (int i = 0; i < 16; i++) {
        out[i] = m[i];
    }
}
