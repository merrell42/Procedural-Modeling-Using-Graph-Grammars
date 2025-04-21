using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEditor.EditorTools;
using UnityEditor.ShortcutManagement;
using UnityEngine;
using System.Runtime.InteropServices;
using System.IO;
using System.Text;

namespace Grammar {
    public class GrammarEditorWindow : EditorWindow {
        [MenuItem("Window/Graph Grammar Generator")] 
        public static void Init() {
            // Get existing open window or if none, make a new one:
            var window = (GrammarEditorWindow)GetWindow(typeof(GrammarEditorWindow));
            window.Show();
        }

#if UNITY_IOS
            // On iOS plugins are statically linked into
            // the executable, so we have to use __Internal as the
            // library name.
            const string dll = "__Internal";
#else
        // Other platforms load plugins dynamically, so pass the
        // name of the plugin's dynamic library.
        const string pmuggDll = "pmugg dll.dll";
#endif  
        [StructLayout(LayoutKind.Sequential)]
        public struct MeshDLL {
            public IntPtr positions;
            public IntPtr normals;
            public IntPtr triangles;
            public int numVertices;
            public int numTriangles;
        }

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern MeshDLL getMesh();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern int initialize(string filePath);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void reset();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void iterate(int steps);

        private void OnEnable() {
            titleContent = new GUIContent("Graph Grammar Generator");
        }

        private string grammarName = "";
        private int iterationCount = 0;
        private bool isAnimating = false;

        private void StartAnimation() {
            if (isAnimating) {
                // Stop current animation first
                EditorApplication.update -= AnimationUpdate;
                isAnimating = false;
            }

            isAnimating = true;
            EditorApplication.update += AnimationUpdate;
        }

        private void AnimationUpdate() {
            if (!isAnimating) {
                EditorApplication.update -= AnimationUpdate;
                Repaint();
                return;
            }

            iterate(1);
            UpdateMesh();
            SceneView.RepaintAll();

            iterationCount++;
            
            Repaint();
        }

        private void UpdateMesh() {
            MeshDLL inputMesh = getMesh();
            Mesh outputMesh = new Mesh();

            int numVertices = inputMesh.numVertices;
            int numTriangles = inputMesh.numTriangles;

            // Marshal the positions array
            float[] positions = new float[numVertices * 3];
            Marshal.Copy(inputMesh.positions, positions, 0, numVertices * 3);
            float[] inputNormals = new float[numVertices * 3];
            Marshal.Copy(inputMesh.normals, inputNormals, 0, numVertices * 3);

            Vector3[] vertices = new Vector3[numVertices];
            Vector3[] normals = new Vector3[numVertices];
            for (int i = 0; i < numVertices; i++) {
                // Switch the y and z coordinates.
                vertices[i] = new Vector3(positions[3 * i], positions[3 * i + 2], positions[3 * i + 1]);
                normals[i] = new Vector3(inputNormals[3 * i], inputNormals[3 * i + 2], inputNormals[3 * i + 1]);
            }
            outputMesh.vertices = vertices;
            outputMesh.normals = normals;

            // Marshal the triangles array
            int[] triangles = new int[3 * numTriangles];
            Marshal.Copy(inputMesh.triangles, triangles, 0, 3 * numTriangles);
            outputMesh.triangles = triangles;

            // outputMesh.RecalculateNormals(); 
            // outputMesh.RecalculateBounds();

            // Check if there's an existing mesh GameObject
            GameObject gameObject = GameObject.Find("Generated Mesh");
            if (gameObject == null) {
                // Create a new GameObject if one doesn't exist
                gameObject = new GameObject("Generated Mesh");                    
                MeshFilter meshFilter = gameObject.AddComponent<MeshFilter>();
                MeshRenderer meshRenderer = gameObject.AddComponent<MeshRenderer>();
                var material = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Material.mat");
                meshRenderer.material = material;
            }
            
            // Update the mesh
            gameObject.GetComponent<MeshFilter>().mesh = outputMesh;
            Selection.activeGameObject = gameObject;
            
            var creator = FindObjectOfType<GrammarCreator>();
            if (creator) {
                gameObject.transform.parent = creator.transform;
            }
        }

        void OnGUI() {
            var r = EditorGUILayout.BeginVertical();
            
            EditorGUILayout.LabelField($"{grammarName}    -     Step: {iterationCount}");

            if (GUILayout.Button("Load Grammar")) {
                string path = EditorUtility.OpenFilePanel("Load Grammar", "", "");
                grammarName = Path.GetFileNameWithoutExtension(path);
                if (!string.IsNullOrEmpty(path)) {
                    initialize(path);
                    iterationCount = 0;
                    UpdateMesh();
                }
            }
            EditorGUI.BeginDisabledGroup(string.IsNullOrEmpty(grammarName));

            if (isAnimating) {
                if (GUILayout.Button("Stop")) {
                    EditorApplication.update -= AnimationUpdate;
                    isAnimating = false;
                }
            } else {
                if (GUILayout.Button("Play")) {
                    StartAnimation();
                }
            }

            if (GUILayout.Button("Reset")) {
                reset();
                iterationCount = 0;
                UpdateMesh();
            }

            if (GUILayout.Button("Step")) {
                iterate(1);
                iterationCount++;
                UpdateMesh();
            }

            EditorGUI.EndDisabledGroup();
            EditorGUILayout.EndFoldoutHeaderGroup();
            EditorGUILayout.EndVertical();
        }
    }
}
