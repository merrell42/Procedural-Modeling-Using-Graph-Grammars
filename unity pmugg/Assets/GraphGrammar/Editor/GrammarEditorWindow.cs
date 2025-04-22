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
            public IntPtr faceIndices;
            public int numVertices;
            public int numTriangles;
            public int numFaces;
        }

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern MeshDLL getMesh();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void initialize(string filePath, StringBuilder result, int len);

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
            int numFaces = inputMesh.numFaces;

            // Marshal the positions array
            float[] positions = new float[numVertices * 3];
            Marshal.Copy(inputMesh.positions, positions, 0, numVertices * 3);
            float[] inputNormals = new float[numVertices * 3];
            Marshal.Copy(inputMesh.normals, inputNormals, 0, numVertices * 3);
            int[] inputFaces = new int[numFaces];
            Marshal.Copy(inputMesh.faceIndices, inputFaces, 0, numFaces);

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
            
            int[] faceIndices = new int[inputMesh.numFaces];
            for (int i = 0; i < numFaces; i++) {
                faceIndices[i] = inputFaces[i];
            }
            // Call the function to draw lines
            DrawEdgeLines(vertices, faceIndices);
        }
        
        private void DrawEdgeLines(Vector3[] vertices, int[] faceIndices) {
            // Clean up existing line renderers
            GameObject linesContainer = GameObject.Find("Generated Lines");
            if (linesContainer != null) {
                DestroyImmediate(linesContainer);
            }
            
            // Create a container for all the lines
            linesContainer = new GameObject("Generated Lines");
            
            // Get the Generated Mesh game object for parenting
            GameObject meshObj = GameObject.Find("Generated Mesh");
            if (meshObj != null) {
                linesContainer.transform.parent = meshObj.transform;
            }
            
            // Example: Draw lines connecting some vertices
            // In a real implementation, you would get the actual edge data from your grammar
            Color lineColor = new Color(0.0f, 0.0f, 0.0f, 1.0f);
            float width = 0.05f;

            int startIndex = 0;
            for (int i = 0; i < faceIndices.Length; i++) {
                int endIndex = faceIndices[i];
                for (int j = startIndex; j < endIndex - 1; j++) {
                    DrawLine(linesContainer, vertices[j], vertices[j + 1], lineColor, width);
                }
                DrawLine(linesContainer, vertices[endIndex - 1], vertices[startIndex], lineColor, width);
                startIndex = endIndex;
            }
        }
        
        private void DrawLine(GameObject parent, Vector3 start, Vector3 end, Color color, float width) {
            GameObject lineObj = new GameObject("Line");
            lineObj.transform.parent = parent.transform;
            
            LineRenderer line = lineObj.AddComponent<LineRenderer>();
            line.material = new Material(Shader.Find("Sprites/Default"));
            line.startColor = color;
            line.endColor = color;
            line.startWidth = width;
            line.endWidth = width;
            line.positionCount = 2;
            line.useWorldSpace = true;
            
            line.SetPosition(0, start);
            line.SetPosition(1, end);
        }

        [Shortcut("Grammar/Play Animation", KeyCode.Space)]
        private static void ToggleAnimationShortcut() {
            var window = GetWindow<GrammarEditorWindow>();
            if (window.isAnimating) {
                window.StopAnimation();
            } else {
                window.StartAnimation();
            }
        }

        private void StopAnimation() {
            EditorApplication.update -= AnimationUpdate;
            isAnimating = false;
        }

        private void ResetGeneration() {
            reset();
            iterationCount = 0;
            UpdateMesh();
        }

        private void IterateSteps(int steps) {
            iterate(steps);
            iterationCount += steps;
            UpdateMesh();
        }

        [Shortcut("Grammar/Reset Generation Keypad", KeyCode.Keypad0)]
        private static void ResetGenerationKeypad() {
            GetWindow<GrammarEditorWindow>().ResetGeneration();
        }

        [Shortcut("Grammar/Reset Generation Alpha", KeyCode.Alpha0)]
        private static void ResetGenerationAlpha() {
            GetWindow<GrammarEditorWindow>().ResetGeneration();
        }

        [Shortcut("Grammar/Step 1 Keypad", KeyCode.Keypad1)]
        private static void Step1Keypad() {
            GetWindow<GrammarEditorWindow>().IterateSteps(1);
        }

        [Shortcut("Grammar/Step 1 Alpha", KeyCode.Alpha1)]
        private static void Step1Alpha() {
            GetWindow<GrammarEditorWindow>().IterateSteps(1);
        }

        [Shortcut("Grammar/Step 2 Keypad", KeyCode.Keypad2)]
        private static void Step2Keypad() {
            GetWindow<GrammarEditorWindow>().IterateSteps(10);
        }

        [Shortcut("Grammar/Step 2 Alpha", KeyCode.Alpha2)]
        private static void Step2Alpha() {
            GetWindow<GrammarEditorWindow>().IterateSteps(10);
        }

        [Shortcut("Grammar/Step 3 Keypad", KeyCode.Keypad3)]
        private static void Step3Keypad() {
            GetWindow<GrammarEditorWindow>().IterateSteps(100);
        }

        [Shortcut("Grammar/Step 3 Alpha", KeyCode.Alpha3)]
        private static void Step3Alpha() {
            GetWindow<GrammarEditorWindow>().IterateSteps(100);
        }

        void OnGUI() {
            var r = EditorGUILayout.BeginVertical();
            
            EditorGUILayout.LabelField($"{grammarName}    -     Step: {iterationCount}");

            if (GUILayout.Button("Load Grammar")) {
                string path = EditorUtility.OpenFilePanel("Load Grammar", "", "");
                if (!string.IsNullOrEmpty(path)) {
                    StringBuilder sb = new StringBuilder(100000);
                    initialize(path, sb, sb.Capacity);
                    Debug.Log(sb);
                    if (sb.ToString() == "Success") {
                        grammarName = Path.GetFileNameWithoutExtension(path);
                        iterationCount = 0;
                        UpdateMesh();
                    } else {
                        grammarName = "";
                        Debug.LogError(sb.ToString());
                    }
                }
            }
            EditorGUI.BeginDisabledGroup(string.IsNullOrEmpty(grammarName));

            if (isAnimating) {
                if (GUILayout.Button("Stop")) {
                    StopAnimation();
                }
            } else if (GUILayout.Button("Play")) {
                StartAnimation();
            }

            if (GUILayout.Button("Reset")) {
                ResetGeneration();
            }

            if (GUILayout.Button("Step")) {
                IterateSteps(1);
            }

            EditorGUI.EndDisabledGroup();
            EditorGUILayout.EndFoldoutHeaderGroup();
            EditorGUILayout.EndVertical();
        }
    }
}
