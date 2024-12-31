#pragma once
#include <array>
#include <cmath>

namespace ms {

// Type alias for matrix/vector storage
using GLArray = std::array<float, 16>;  // Large enough for all types

// Vector3 operations
namespace vec3 {
    GLArray create(const GLArray* data = nullptr);
    void set(const GLArray& src, GLArray& dest);
    void add(const GLArray& a, const GLArray& b, GLArray& dest);
    void subtract(const GLArray& a, const GLArray& b, GLArray& dest);
    void negate(const GLArray& a, GLArray& dest);
    void scale(const GLArray& a, float b, GLArray& dest);
    void normalize(const GLArray& a, GLArray& dest);
    void cross(const GLArray& a, const GLArray& b, GLArray& dest);
    float length(const GLArray& a);
    float dot(const GLArray& a, const GLArray& b);
    void direction(const GLArray& a, const GLArray& b, GLArray& dest);
    void lerp(const GLArray& a, const GLArray& b, float t, GLArray& dest);
}

// Matrix3 operations
namespace mat3 {
    GLArray create(const GLArray* data = nullptr);
    void set(const GLArray& src, GLArray& dest);
    void identity(GLArray& dest);
    void transpose(const GLArray& a, GLArray& dest);
    void toMat4(const GLArray& a, GLArray& dest);
}

// Matrix4 operations
namespace mat4 {
    GLArray create(const GLArray* data = nullptr);
    void set(const GLArray& src, GLArray& dest);
    void identity(GLArray& dest);
    void transpose(const GLArray& a, GLArray& dest);
    float determinant(const GLArray& a);
    void inverse(const GLArray& a, GLArray& dest);
    void toRotationMat(const GLArray& a, GLArray& dest);
    void toMat3(const GLArray& a, GLArray& dest);
    void toInverseMat3(const GLArray& a, GLArray& dest);
    void multiply(const GLArray& a, const GLArray& b, GLArray& dest);
    void multiplyVec3(const GLArray& mat, const GLArray& vec, GLArray& dest);
    void multiplyVec4(const GLArray& mat, const GLArray& vec, GLArray& dest);
    void translate(const GLArray& mat, const GLArray& vec, GLArray& dest);
    void scale(const GLArray& mat, const GLArray& vec, GLArray& dest);
    void rotate(const GLArray& mat, float angle, const GLArray& axis, GLArray& dest);
    void rotateX(const GLArray& mat, float angle, GLArray& dest);
    void rotateY(const GLArray& mat, float angle, GLArray& dest);
    void rotateZ(const GLArray& mat, float angle, GLArray& dest);
    void frustum(float left, float right, float bottom, float top, float near, float far, GLArray& dest);
    void perspective(float fovy, float aspect, float near, float far, GLArray& dest);
    void ortho(float left, float right, float bottom, float top, float near, float far, GLArray& dest);
    void lookAt(const GLArray& eye, const GLArray& center, const GLArray& up, GLArray& dest);
}

// Quaternion operations
namespace quat4 {
    GLArray create(const GLArray* data = nullptr);
    void set(const GLArray& src, GLArray& dest);
    void calculateW(const GLArray& quat, GLArray& dest);
    void inverse(const GLArray& quat, GLArray& dest);
    float length(const GLArray& quat);
    void normalize(const GLArray& quat, GLArray& dest);
    void multiply(const GLArray& a, const GLArray& b, GLArray& dest);
    void multiplyVec3(const GLArray& quat, const GLArray& vec, GLArray& dest);
    void toMat3(const GLArray& quat, GLArray& dest);
    void toMat4(const GLArray& quat, GLArray& dest);
    void slerp(const GLArray& a, const GLArray& b, float t, GLArray& dest);
}

} // namespace ms 