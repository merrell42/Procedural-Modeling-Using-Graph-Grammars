#pragma once
#include <array>
#include <vector>

namespace ms {

class m4 {
public:
    using Vector3 = std::array<float, 3>;
    using Vector4 = std::array<float, 4>;
    using Matrix4 = std::array<float, 16>;

    // Matrix creation
    static Matrix4 multiply(const Matrix4& a, const Matrix4& b, Matrix4* dst = nullptr);
    static Vector3 addVectors(const Vector3& a, const Vector3& b, Vector3* dst = nullptr);
    static Vector3 subtractVectors(const Vector3& a, const Vector3& b, Vector3* dst = nullptr);
    static Vector3 scaleVector(const Vector3& v, float s, Vector3* dst = nullptr);
    static Vector3 normalize(const Vector3& v, Vector3* dst = nullptr);
    static float length(const Vector3& v);
    static float lengthSq(const Vector3& v);
    static Vector3 cross(const Vector3& a, const Vector3& b, Vector3* dst = nullptr);
    static float dot(const Vector3& a, const Vector3& b);
    static float distanceSq(const Vector3& a, const Vector3& b);
    static float distance(const Vector3& a, const Vector3& b);
    static Matrix4 identity(Matrix4* dst = nullptr);
    static Matrix4 transpose(const Matrix4& m, Matrix4* dst = nullptr);
    static Matrix4 inverse(const Matrix4& m, Matrix4* dst = nullptr);
    static Matrix4 translation(const Vector3& v, Matrix4* dst = nullptr);
    static Matrix4 translationFromValues(float tx, float ty, float tz, Matrix4* dst = nullptr);
    static Matrix4 translate(const Matrix4& m, const Vector3& v, Matrix4* dst = nullptr);
    static Matrix4 rotation(float angleInRadians, const Vector3& axis, Matrix4* dst = nullptr);
    static Matrix4 rotationFromValues(float angleInRadians, float x, float y, float z, Matrix4* dst = nullptr);
    static Matrix4 rotate(const Matrix4& m, float angleInRadians, const Vector3& axis, Matrix4* dst = nullptr);
    static Matrix4 rotationX(float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 rotateX(const Matrix4& m, float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 rotationY(float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 rotateY(const Matrix4& m, float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 rotationZ(float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 rotateZ(const Matrix4& m, float angleInRadians, Matrix4* dst = nullptr);
    static Matrix4 scaling(const Vector3& v, Matrix4* dst = nullptr);
    static Matrix4 scalingFromValues(float sx, float sy, float sz, Matrix4* dst = nullptr);
    static Matrix4 scale(const Matrix4& m, const Vector3& v, Matrix4* dst = nullptr);
    static Matrix4 perspective(float fieldOfViewYInRadians, float aspect, float zNear, float zFar, Matrix4* dst = nullptr);
    static Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far, Matrix4* dst = nullptr);
    static Matrix4 lookAt(const Vector3& cameraPosition, const Vector3& target, const Vector3& up, Matrix4* dst = nullptr);
    static Matrix4 transformPoint(const Matrix4& m, const Vector3& v, Vector3* dst = nullptr);
    static Matrix4 transformDirection(const Matrix4& m, const Vector3& v, Vector3* dst = nullptr);
    static Matrix4 transformNormal(const Matrix4& m, const Vector3& v, Vector3* dst = nullptr);

private:
    static Matrix4 defaultMatrix();
    static Vector3 defaultVector3();
};

} // namespace ms 