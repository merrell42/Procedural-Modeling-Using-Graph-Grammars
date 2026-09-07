using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEditor;
using UnityEngine;

namespace Grammar {
    public static class GrammarDllMesh {
        private const string dll = "pmugg release.dll";

        [StructLayout(LayoutKind.Sequential)]
        public struct SubmeshCpp {
            public IntPtr positions;
            public IntPtr normals;
            public IntPtr triangles;
            public IntPtr faceIndices;
            public int numVertices;
            public int numTriangles;
            public int numFaces;
            public float red;
            public float green;
            public float blue;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MeshCpp {
            public IntPtr submeshes;
            public int numSubmeshes;
        }

        [DllImport(dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern int getNumProductionRules(int category);

        [DllImport(dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern int getProductionRuleGraphCount(int category, int ruleIndex);

        [DllImport(dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern MeshCpp getProductionRuleGraphMesh(int category, int ruleIndex, int graphIndex);

        [DllImport(dll, CallingConvention = CallingConvention.Cdecl)]
        public static extern void destroyMesh(ref MeshCpp mesh);

        public static Vector3 ToUnityPosition(float[] positions, int vertexIndex) {
            int i = vertexIndex * 3;
            return new Vector3(positions[i], positions[i + 2], positions[i + 1]);
        }

        public static Vector3 ToUnityNormal(float[] normals, int vertexIndex) {
            int i = vertexIndex * 3;
            return new Vector3(normals[i], normals[i + 2], normals[i + 1]);
        }

        static bool IsEdgeLineSubmesh(SubmeshCpp submesh) {
            return submesh.red == 0f && submesh.green == 0f && submesh.blue == 0f;
        }

        public static Material CreateMaterial(SubmeshCpp submesh) {
            var material = new Material(Shader.Find("Standard"));
            material.color = new Color(submesh.red, submesh.green, submesh.blue, 1.0f);
            return material;
        }

        static void AppendSubmeshVertices(
            SubmeshCpp submesh,
            float[] positions,
            float[] normalValues,
            bool doubleSided,
            List<Vector3> vertices,
            List<Vector3> normals
        ) {
            for (int i = 0; i < submesh.numVertices; i++) {
                vertices.Add(ToUnityPosition(positions, i));
                if (normalValues != null) {
                    normals.Add(ToUnityNormal(normalValues, i));
                } else {
                    normals.Add(Vector3.up);
                }
            }

            if (!doubleSided) {
                return;
            }

            int frontVertexCount = submesh.numVertices;
            for (int i = 0; i < frontVertexCount; i++) {
                vertices.Add(ToUnityPosition(positions, i));
                if (normalValues != null) {
                    normals.Add(-ToUnityNormal(normalValues, i));
                } else {
                    normals.Add(-Vector3.up);
                }
            }
        }

        static int[] BuildTriangles(int[] triangleValues, int frontVertexOffset, int frontVertexCount, bool doubleSided) {
            for (int i = 0; i < triangleValues.Length; i++) {
                triangleValues[i] += frontVertexOffset;
            }

            if (!doubleSided) {
                return triangleValues;
            }

            int backVertexOffset = frontVertexOffset + frontVertexCount;
            var doubled = new int[triangleValues.Length * 2];
            Array.Copy(triangleValues, doubled, triangleValues.Length);

            int backTriangleOffset = triangleValues.Length;
            for (int t = 0; t < triangleValues.Length; t += 3) {
                int localA = triangleValues[t] - frontVertexOffset;
                int localB = triangleValues[t + 1] - frontVertexOffset;
                int localC = triangleValues[t + 2] - frontVertexOffset;

                doubled[backTriangleOffset + t] = backVertexOffset + localA;
                doubled[backTriangleOffset + t + 1] = backVertexOffset + localC;
                doubled[backTriangleOffset + t + 2] = backVertexOffset + localB;
            }

            return doubled;
        }

        public static GameObject BuildGameObject(MeshCpp meshData, string objectName) {
            var vertices = new List<Vector3>();
            var normals = new List<Vector3>();
            var submeshTriangleLists = new List<int[]>();
            var materials = new List<Material>();

            if (meshData.submeshes != IntPtr.Zero) {
                int submeshStructSize = Marshal.SizeOf<SubmeshCpp>();
                for (int submeshIndex = 0; submeshIndex < meshData.numSubmeshes; submeshIndex++) {
                    IntPtr submeshPtr = IntPtr.Add(meshData.submeshes, submeshIndex * submeshStructSize);
                    SubmeshCpp submesh = Marshal.PtrToStructure<SubmeshCpp>(submeshPtr);

                    int frontVertexOffset = vertices.Count;
                    var positions = new float[submesh.numVertices * 3];
                    Marshal.Copy(submesh.positions, positions, 0, positions.Length);

                    float[] normalValues = null;
                    if (submesh.normals != IntPtr.Zero) {
                        normalValues = new float[submesh.numVertices * 3];
                        Marshal.Copy(submesh.normals, normalValues, 0, normalValues.Length);
                    }

                    bool doubleSided = !IsEdgeLineSubmesh(submesh);
                    AppendSubmeshVertices(submesh, positions, normalValues, doubleSided, vertices, normals);

                    if (submesh.numTriangles > 0) {
                        var triangleValues = new int[submesh.numTriangles * 3];
                        Marshal.Copy(submesh.triangles, triangleValues, 0, triangleValues.Length);
                        submeshTriangleLists.Add(
                            BuildTriangles(triangleValues, frontVertexOffset, submesh.numVertices, doubleSided));
                        materials.Add(CreateMaterial(submesh));
                    }
                }
            }

            destroyMesh(ref meshData);

            var gameObject = new GameObject(objectName);
            if (vertices.Count == 0 || submeshTriangleLists.Count == 0) {
                return gameObject;
            }

            var outputMesh = new Mesh();
            if (vertices.Count > 65535) {
                outputMesh.indexFormat = UnityEngine.Rendering.IndexFormat.UInt32;
            }
            outputMesh.SetVertices(vertices);
            outputMesh.SetNormals(normals);
            outputMesh.subMeshCount = submeshTriangleLists.Count;
            for (int submeshIndex = 0; submeshIndex < submeshTriangleLists.Count; submeshIndex++) {
                outputMesh.SetTriangles(submeshTriangleLists[submeshIndex], submeshIndex);
            }
            outputMesh.RecalculateBounds();

            var meshFilter = gameObject.AddComponent<MeshFilter>();
            var meshRenderer = gameObject.AddComponent<MeshRenderer>();
            meshFilter.sharedMesh = outputMesh;
            meshRenderer.sharedMaterials = materials.ToArray();
            meshRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
            meshRenderer.receiveShadows = true;
            return gameObject;
        }
    }
}
