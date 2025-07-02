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
        private const int MAX_ITERATION = 200;

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
        const string pmuggDll = "pmugg release.dll";
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
        private static extern void destroyMesh(ref MeshDLL mesh);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void initialize(string filePath, StringBuilder result, int len, int seed);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void reset(int seed);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void iterate(int steps);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern int iterateToTime(float timeSeconds);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void setSize(float x, float y, float z);

        private static Material sharedLineMaterial = null;

        private void OnEnable() {
            titleContent = new GUIContent("Graph Grammar Generator");
        }

        private Queue<string> fileQueue = new Queue<string>();
        private int maxIteration = MAX_ITERATION;
        private string grammarName = "";
        private int iterationCount = 0;
        private bool isAnimating = false;
        private int seed = 0;
        private Vector3 size = new Vector3(30, 20, 10);
        private Vector3 previousSize;
        private System.Diagnostics.Stopwatch timer = new System.Diagnostics.Stopwatch();

        // Reusable arrays to prevent garbage collection pressure
        private float[] positionsArray;
        private float[] normalsArray;
        private int[] trianglesArray;
        private int[] faceIndicesArray;
        private Vector3[] verticesArray;
        private Vector3[] normalsVectorArray;

        private void HandleSizeChange() {
            setSize(size.x, size.y, size.z);
        }

        private void StartAnimation() {
            if (isAnimating) {
                // Stop current animation first
                EditorApplication.update -= AnimationUpdate;
                isAnimating = false;
            }

            timer.Start();
            isAnimating = true;
            EditorApplication.update += AnimationUpdate;
        }

        private void AnimationUpdate() {
            if (!isAnimating) {
                EditorApplication.update -= AnimationUpdate;
                Repaint();
                return;
            }
            if (iterationCount >= maxIteration && maxIteration > 0) {
                if (fileQueue.Count > 0) {
                    iterationCount = 0;
                    LoadGrammarFile(fileQueue.Dequeue());
                } else {
                    StopAnimation();
                    Repaint();
                    return;
                }
            }
            
            float timePerFrame = 0.1f;
            int stepsCompleted = iterateToTime(timePerFrame);
            iterationCount += stepsCompleted;

            UpdateMesh();
            SceneView.RepaintAll();
            
            Repaint();
        }

        private void UpdateMesh() {
            MeshDLL inputMesh = getMesh();
            Mesh outputMesh = new Mesh();

            int numVertices = inputMesh.numVertices;
            int numTriangles = inputMesh.numTriangles;
            int numFaces = inputMesh.numFaces;

            // Marshal the positions array
            if (positionsArray == null || positionsArray.Length < numVertices * 3) {
                positionsArray = new float[numVertices * 3];
            } else {
                Array.Clear(positionsArray, 0, positionsArray.Length);
            }
            Marshal.Copy(inputMesh.positions, positionsArray, 0, numVertices * 3);
            
            if (normalsArray == null || normalsArray.Length < numVertices * 3) {
                normalsArray = new float[numVertices * 3];
            } else {
                Array.Clear(normalsArray, 0, normalsArray.Length);
            }
            Marshal.Copy(inputMesh.normals, normalsArray, 0, numVertices * 3);
            
            if (faceIndicesArray == null || faceIndicesArray.Length < numFaces) {
                faceIndicesArray = new int[numFaces];
            } else {
                Array.Clear(faceIndicesArray, 0, faceIndicesArray.Length);
            }
            Marshal.Copy(inputMesh.faceIndices, faceIndicesArray, 0, numFaces);

            if (verticesArray == null || verticesArray.Length < numVertices) {
                verticesArray = new Vector3[numVertices];
            } else {
                Array.Clear(verticesArray, 0, verticesArray.Length);
            }
            if (normalsVectorArray == null || normalsVectorArray.Length < numVertices) {
                normalsVectorArray = new Vector3[numVertices];
            } else {
                Array.Clear(normalsVectorArray, 0, normalsVectorArray.Length);
            }
            for (int i = 0; i < numVertices; i++) {
                // Switch the y and z coordinates.
                verticesArray[i] = new Vector3(positionsArray[3 * i], positionsArray[3 * i + 2], positionsArray[3 * i + 1]);
                normalsVectorArray[i] = new Vector3(normalsArray[3 * i], normalsArray[3 * i + 2], normalsArray[3 * i + 1]);
            }
            outputMesh.vertices = verticesArray;
            outputMesh.normals = normalsVectorArray;

            // Marshal the triangles array
            if (trianglesArray == null || trianglesArray.Length < 3 * numTriangles) {
                trianglesArray = new int[3 * numTriangles];
            } else {
                Array.Clear(trianglesArray, 0, trianglesArray.Length);
            }
            Marshal.Copy(inputMesh.triangles, trianglesArray, 0, 3 * numTriangles);
            outputMesh.triangles = trianglesArray;

            // Free the C++ memory after copying all data
            destroyMesh(ref inputMesh);

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
            
            // Clean up the old mesh before assigning the new one
            MeshFilter existingMeshFilter = gameObject.GetComponent<MeshFilter>();
            if (existingMeshFilter.sharedMesh != null) {
                // Destroy the old mesh to free memory
                if (Application.isPlaying) {
                    Destroy(existingMeshFilter.sharedMesh);
                } else {
                    DestroyImmediate(existingMeshFilter.sharedMesh);
                }
            }
            
            // Update the mesh
            existingMeshFilter.sharedMesh = outputMesh;
            Selection.activeGameObject = gameObject;
            
            var creator = FindFirstObjectByType<GrammarCreator>();
            if (creator) {
                gameObject.transform.parent = creator.transform;
            }
            
            int[] faceIndices = new int[numFaces];
            for (int i = 0; i < numFaces; i++) {
                faceIndices[i] = faceIndicesArray[i];
            }
            // Call the function to draw lines
            DrawEdgeLines(verticesArray, faceIndices);
            
            // Force Garbage Collection every 20 iterations to prevent memory buildup.
            // Not sure how helpful this is.
            if (iterationCount % 20 == 0) {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
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
            
            // Use a shared material to prevent memory leaks
            if (sharedLineMaterial == null) {
                sharedLineMaterial = new Material(Shader.Find("Sprites/Default"));
            }
            line.material = sharedLineMaterial;
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
            timer.Stop();
            isAnimating = false;
        }

        private void ResetGeneration() {
            timer.Reset();
            reset(seed);
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
            EditorGUILayout.BeginVertical();

            string stepText = maxIteration > 0 ? $"Step: {iterationCount} / {maxIteration}" : $"Step: {iterationCount}";
            string timerText = $"    -     {timer.Elapsed.TotalSeconds:F1}s";
            if (iterationCount > 0) {
                timerText += $" ({1000 * timer.Elapsed.TotalSeconds / iterationCount:F3} ms/step)";
            }
            EditorGUILayout.LabelField($"{grammarName}    -     {stepText}{timerText}");

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("Seed", GUILayout.Width(50));
            seed = EditorGUILayout.IntField(seed);
            EditorGUILayout.LabelField("Size", GUILayout.Width(50));
            EditorGUI.BeginChangeCheck();
            size.x = EditorGUILayout.FloatField(size.x);
            size.y = EditorGUILayout.FloatField(size.y);
            size.z = EditorGUILayout.FloatField(size.z);
            if (EditorGUI.EndChangeCheck()) {
                HandleSizeChange();
            }
            EditorGUILayout.EndHorizontal();

            if (GUILayout.Button("Load Grammar")) {
                string path = EditorUtility.OpenFilePanel("Load Grammar", "", "");
                if (!string.IsNullOrEmpty(path)) {
                    maxIteration = 0;
                    LoadGrammarFile(path);
                }
            }
            
            if (GUILayout.Button("Load Folder")) {
                string folderPath = EditorUtility.OpenFolderPanel("Load Grammar Folder", "", "");
                if (!string.IsNullOrEmpty(folderPath)) {
                    LoadGrammarFolder(folderPath);
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
        
        private void LoadGrammarFile(string path) {
            StringBuilder sb = new StringBuilder(100000);
            initialize(path, sb, sb.Capacity, seed);
            if (sb.ToString() == "Success") {
                grammarName = Path.GetFileNameWithoutExtension(path);
                iterationCount = 0;
                UpdateMesh();
            } else {
                grammarName = "";
                Debug.LogError(sb.ToString());
            }
        }
        
        private void LoadGrammarFolder(string folderPath) {
            // Get all JSON files in the folder and its subfolders
            fileQueue = new Queue<string>(Directory.GetFiles(folderPath, "*.json", SearchOption.AllDirectories));
            
            if (fileQueue.Count == 0) {
                Debug.LogWarning("No JSON files found in the selected folder.");
                return;
            }

            maxIteration = MAX_ITERATION;
            LoadGrammarFile(fileQueue.Dequeue());
            StartAnimation();
        }
        
        private string GetRelativePath(string rootPath, string fullPath) {
            // Remove the root path to get a relative path for display in the menu
            string relativePath = fullPath.Substring(rootPath.Length);
            // Remove leading slash or backslash if present
            if (relativePath.StartsWith("/") || relativePath.StartsWith("\\")) {
                relativePath = relativePath.Substring(1);
            }
            return relativePath;
        }
    }
}
