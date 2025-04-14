using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace Grammar {
    // [AddComponentMenu("Grammar/Face Set")]
    public class FaceSet : MonoBehaviour {
        public static int idCounter = 0;

        // A unique ID.
        public int id = idCounter++;
        // [Tooltip("The name of the face set.")]
        // public string faceName;

        [Tooltip("The number of sides of new face set.")]
        public int sides = 4;

        [Tooltip("The angle with respect to the ground. 0 is parallel to the ground. 90 is vertical.")]
        public float angle = 90;

        // The same face rotated to each side.
        public List<FaceType> types = new List<FaceType>();

        public Vector3 GetNormal(int sideNum) {
            var theta = (float)sideNum / (float)sides * 360;
            var phi = angle * Mathf.PI / 180f;
            var vector = new Vector3(Mathf.Sin(phi), Mathf.Cos(phi), 0);
            return RotateAroundYAxis(vector, theta);
        }

        public bool isFlat() {
            return angle == 0 || angle == 180;
        }

        public static Vector3 RotateAroundYAxis(Vector3 vector, float angleInDegrees) {
            // Create a rotation quaternion
            Quaternion rotationQuaternion = Quaternion.Euler(0f, angleInDegrees, 0f);

            // Apply the rotation to the vector
            Vector3 rotatedVector = rotationQuaternion * vector;

            return rotatedVector;
        }
    }
}