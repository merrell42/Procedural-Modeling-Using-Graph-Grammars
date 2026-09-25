using System.Runtime.InteropServices;
using UnityEngine;

namespace Grammar {
    // Must match cpp_version/geometry/matrix4.h. Column-major: element (row, col) is m[col * 4 + row].
    [StructLayout(LayoutKind.Sequential)]
    public struct Matrix4 {
        public float m0, m1, m2, m3;
        public float m4, m5, m6, m7;
        public float m8, m9, m10, m11;
        public float m12, m13, m14, m15;

        public static readonly Matrix4 Identity = new Matrix4 {
            m0 = 1f,
            m5 = 1f,
            m10 = 1f,
            m15 = 1f,
        };

        // Swaps grammar Y/Z into Unity's coordinate system.
        public static readonly Matrix4 GrammarToUnity = new Matrix4 {
            m0 = 1f,
            m5 = 0f,
            m6 = 1f,
            m7 = 0f,
            m9 = 1f,
            m10 = 0f,
            m15 = 1f,
        };

        public static Matrix4 operator *(Matrix4 left, Matrix4 right) {
            Matrix4 result = default;
            for (int col = 0; col < 4; col++) {
                for (int row = 0; row < 4; row++) {
                    float sum = 0f;
                    for (int k = 0; k < 4; k++) {
                        sum += left.At(k, row) * right.At(col, k);
                    }
                    result.Set(col, row, sum);
                }
            }
            return result;
        }

        public void Decompose(out Vector3 position, out Quaternion rotation, out Vector3 scale) {
            position = new Vector3(m12, m13, m14);

            Vector3 column0 = new Vector3(m0, m1, m2);
            Vector3 column1 = new Vector3(m4, m5, m6);
            Vector3 column2 = new Vector3(m8, m9, m10);

            scale = new Vector3(column0.magnitude, column1.magnitude, column2.magnitude);

            if (scale.x > 1e-6f) {
                column0 /= scale.x;
            }
            if (scale.y > 1e-6f) {
                column1 /= scale.y;
            }
            if (scale.z > 1e-6f) {
                column2 /= scale.z;
            }

            var rotationMatrix = new Matrix4x4(column0, column1, column2, new Vector4(0f, 0f, 0f, 1f));
            rotation = rotationMatrix.rotation;
        }

        public void ApplyTo(Transform target) {
            Matrix4 unityMatrix = GrammarToUnity * this * GrammarToUnity;
            unityMatrix.Decompose(out Vector3 position, out Quaternion rotation, out Vector3 scale);
            target.localPosition = position;
            target.localRotation = rotation;
            target.localScale = scale;
        }

        float At(int col, int row) {
            return this[col * 4 + row];
        }

        void Set(int col, int row, float value) {
            this[col * 4 + row] = value;
        }

        float this[int index] {
            get {
                switch (index) {
                    case 0: return m0;
                    case 1: return m1;
                    case 2: return m2;
                    case 3: return m3;
                    case 4: return m4;
                    case 5: return m5;
                    case 6: return m6;
                    case 7: return m7;
                    case 8: return m8;
                    case 9: return m9;
                    case 10: return m10;
                    case 11: return m11;
                    case 12: return m12;
                    case 13: return m13;
                    case 14: return m14;
                    case 15: return m15;
                    default: return 0f;
                }
            }
            set {
                switch (index) {
                    case 0: m0 = value; break;
                    case 1: m1 = value; break;
                    case 2: m2 = value; break;
                    case 3: m3 = value; break;
                    case 4: m4 = value; break;
                    case 5: m5 = value; break;
                    case 6: m6 = value; break;
                    case 7: m7 = value; break;
                    case 8: m8 = value; break;
                    case 9: m9 = value; break;
                    case 10: m10 = value; break;
                    case 11: m11 = value; break;
                    case 12: m12 = value; break;
                    case 13: m13 = value; break;
                    case 14: m14 = value; break;
                    case 15: m15 = value; break;
                }
            }
        }
    }
}
