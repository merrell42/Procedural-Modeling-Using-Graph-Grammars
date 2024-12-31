#include "gl_matrix.h"
#include <cstring>

namespace ms {

namespace vec3 {

GLArray create(const GLArray* data) {
    GLArray result = {0, 0, 0};
    if (data) {
        result[0] = (*data)[0];
        result[1] = (*data)[1];
        result[2] = (*data)[2];
    }
    return result;
}

void set(const GLArray& src, GLArray& dest) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

void add(const GLArray& a, const GLArray& b, GLArray& dest) {
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

void subtract(const GLArray& a, const GLArray& b, GLArray& dest) {
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

void negate(const GLArray& a, GLArray& dest) {
    dest[0] = -a[0];
    dest[1] = -a[1];
    dest[2] = -a[2];
}

void scale(const GLArray& a, float b, GLArray& dest) {
    dest[0] = a[0] * b;
    dest[1] = a[1] * b;
    dest[2] = a[2] * b;
}

void normalize(const GLArray& a, GLArray& dest) {
    float len = length(a);
    if (len) {
        float invLen = 1.0f / len;
        dest[0] = a[0] * invLen;
        dest[1] = a[1] * invLen;
        dest[2] = a[2] * invLen;
    } else {
        dest[0] = dest[1] = dest[2] = 0;
    }
}

void cross(const GLArray& a, const GLArray& b, GLArray& dest) {
    dest[0] = a[1] * b[2] - a[2] * b[1];
    dest[1] = a[2] * b[0] - a[0] * b[2];
    dest[2] = a[0] * b[1] - a[1] * b[0];
}

float length(const GLArray& a) {
    return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

float dot(const GLArray& a, const GLArray& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// ... (similar implementations for remaining vec3 functions)
} // namespace vec3

namespace mat3 {
// ... (implementations for mat3 functions)
} // namespace mat3

namespace mat4 {

GLArray create(const GLArray* data) {
    GLArray result = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    if (data) {
        std::copy(data->begin(), data->end(), result.begin());
    }
    return result;
}

void multiply(const GLArray& a, const GLArray& b, GLArray& dest) {
    float a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
    float a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
    float a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
    float a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

    float b00 = b[0], b01 = b[1], b02 = b[2], b03 = b[3];
    float b10 = b[4], b11 = b[5], b12 = b[6], b13 = b[7];
    float b20 = b[8], b21 = b[9], b22 = b[10], b23 = b[11];
    float b30 = b[12], b31 = b[13], b32 = b[14], b33 = b[15];

    dest[0] = b00*a00 + b01*a10 + b02*a20 + b03*a30;
    dest[1] = b00*a01 + b01*a11 + b02*a21 + b03*a31;
    dest[2] = b00*a02 + b01*a12 + b02*a22 + b03*a32;
    dest[3] = b00*a03 + b01*a13 + b02*a23 + b03*a33;
    dest[4] = b10*a00 + b11*a10 + b12*a20 + b13*a30;
    dest[5] = b10*a01 + b11*a11 + b12*a21 + b13*a31;
    dest[6] = b10*a02 + b11*a12 + b12*a22 + b13*a32;
    dest[7] = b10*a03 + b11*a13 + b12*a23 + b13*a33;
    dest[8] = b20*a00 + b21*a10 + b22*a20 + b23*a30;
    dest[9] = b20*a01 + b21*a11 + b22*a21 + b23*a31;
    dest[10] = b20*a02 + b21*a12 + b22*a22 + b23*a32;
    dest[11] = b20*a03 + b21*a13 + b22*a23 + b23*a33;
    dest[12] = b30*a00 + b31*a10 + b32*a20 + b33*a30;
    dest[13] = b30*a01 + b31*a11 + b32*a21 + b33*a31;
    dest[14] = b30*a02 + b31*a12 + b32*a22 + b33*a32;
    dest[15] = b30*a03 + b31*a13 + b32*a23 + b33*a33;
}

// ... (implementations for remaining mat4 functions)
} // namespace mat4

namespace quat4 {
// ... (implementations for quat4 functions)
} // namespace quat4

} // namespace ms 