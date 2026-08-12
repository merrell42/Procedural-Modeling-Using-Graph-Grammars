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
        // Must match cpp_version/geometry/mesh.h (SubmeshCpp, MeshCpp).
        [StructLayout(LayoutKind.Sequential)]
        private struct SubmeshCpp {
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
        private struct MeshCpp {
            public IntPtr submeshes;
            public int numSubmeshes;
        }

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern MeshCpp getMesh();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void destroyMesh(ref MeshCpp mesh);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void initialize(string filePath, StringBuilder result, int len, int seed);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void getLastWarning(StringBuilder result, int len);

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

        private Material[] lastMeshMaterials;

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
            LogDllWarning();

            UpdateMesh();
            SceneView.RepaintAll();
            
            Repaint();
        }

        private void UpdateMesh() {
            MeshCpp meshData = getMesh();

            var vertices = new List<Vector3>();
            var normals = new List<Vector3>();
            var submeshTriangleLists = new List<int[]>();
            var materials = new List<Material>();
            Vector3[] edgeVertices = null;
            int[] edgeFaceIndices = null;

            if (meshData.submeshes != IntPtr.Zero) {
                int submeshStructSize = Marshal.SizeOf<SubmeshCpp>();
                for (int submeshIndex = 0; submeshIndex < meshData.numSubmeshes; submeshIndex++) {
                    IntPtr submeshPtr = IntPtr.Add(meshData.submeshes, submeshIndex * submeshStructSize);
                    SubmeshCpp submesh = Marshal.PtrToStructure<SubmeshCpp>(submeshPtr);

                    int vertexOffset = vertices.Count;
                    var positions = new float[submesh.numVertices * 3];
                    Marshal.Copy(submesh.positions, positions, 0, positions.Length);

                    if (submesh.normals != IntPtr.Zero) {
                        var normalValues = new float[submesh.numVertices * 3];
                        Marshal.Copy(submesh.normals, normalValues, 0, normalValues.Length);
                        for (int i = 0; i < submesh.numVertices; i++) {
                            vertices.Add(ToUnityPosition(positions, i));
                            normals.Add(ToUnityNormal(normalValues, i));
                        }
                    } else {
                        for (int i = 0; i < submesh.numVertices; i++) {
                            vertices.Add(ToUnityPosition(positions, i));
                            normals.Add(Vector3.up);
                        }
                    }

                    if (submesh.numTriangles > 0) {
                        var triangleValues = new int[submesh.numTriangles * 3];
                        Marshal.Copy(submesh.triangles, triangleValues, 0, triangleValues.Length);
                        for (int i = 0; i < triangleValues.Length; i++) {
                            triangleValues[i] += vertexOffset;
                        }
                        submeshTriangleLists.Add(triangleValues);
                        materials.Add(CreateSubmeshMaterial(submesh));
                    }

                    if (edgeVertices == null && submesh.numFaces > 0) {
                        edgeVertices = new Vector3[submesh.numVertices];
                        for (int i = 0; i < submesh.numVertices; i++) {
                            edgeVertices[i] = ToUnityPosition(positions, i);
                        }
                        edgeFaceIndices = new int[submesh.numFaces];
                        Marshal.Copy(submesh.faceIndices, edgeFaceIndices, 0, submesh.numFaces);
                    }
                }
            }

            destroyMesh(ref meshData);

            if (vertices.Count == 0 && edgeVertices == null) {
                DestroyMeshMaterials();
                ClearGeneratedObjects();
                return;
            }

            GameObject gameObject = GameObject.Find("Generated Mesh");
            if (gameObject == null) {
                gameObject = new GameObject("Generated Mesh");
                gameObject.AddComponent<MeshFilter>();
                gameObject.AddComponent<MeshRenderer>();
            }

            if (submeshTriangleLists.Count > 0) {
                Mesh outputMesh = new Mesh();
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

                MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
                MeshRenderer meshRenderer = gameObject.GetComponent<MeshRenderer>();

                if (meshFilter.sharedMesh != null) {
                    DestroyImmediate(meshFilter.sharedMesh);
                }

                DestroyMeshMaterials();
                lastMeshMaterials = materials.ToArray();
                meshFilter.sharedMesh = outputMesh;
                meshRenderer.sharedMaterials = lastMeshMaterials;
            } else {
                MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
                MeshRenderer meshRenderer = gameObject.GetComponent<MeshRenderer>();
                if (meshFilter.sharedMesh != null) {
                    DestroyImmediate(meshFilter.sharedMesh);
                    meshFilter.sharedMesh = null;
                }
                DestroyMeshMaterials();
                meshRenderer.sharedMaterials = Array.Empty<Material>();
            }

            Selection.activeGameObject = gameObject;

            var creator = FindFirstObjectByType<GrammarCreator>();
            if (creator) {
                gameObject.transform.parent = creator.transform;
            }

            if (edgeVertices != null && edgeFaceIndices != null) {
                DrawEdgeLines(edgeVertices, edgeFaceIndices);
            }

            if (iterationCount % 20 == 0) {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
        }
        
        private static Vector3 ToUnityPosition(float[] positions, int vertexIndex) {
            int i = vertexIndex * 3;
            return new Vector3(positions[i], positions[i + 2], positions[i + 1]);
        }

        private static Vector3 ToUnityNormal(float[] normals, int vertexIndex) {
            int i = vertexIndex * 3;
            return new Vector3(normals[i], normals[i + 2], normals[i + 1]);
        }

        private static Material CreateSubmeshMaterial(SubmeshCpp submesh) {
            var material = new Material(Shader.Find("Standard"));
            material.color = new Color(submesh.red, submesh.green, submesh.blue, 1.0f);
            return material;
        }

        private void DestroyMeshMaterials() {
            if (lastMeshMaterials == null) {
                return;
            }
            foreach (Material material in lastMeshMaterials) {
                if (material != null) {
                    DestroyImmediate(material);
                }
            }
            lastMeshMaterials = null;
        }

        private void ClearGeneratedObjects() {
            GameObject linesContainer = GameObject.Find("Generated Lines");
            if (linesContainer != null) {
                DestroyImmediate(linesContainer);
            }

            GameObject gameObject = GameObject.Find("Generated Mesh");
            if (gameObject == null) {
                return;
            }

            MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
            if (meshFilter != null && meshFilter.sharedMesh != null) {
                DestroyImmediate(meshFilter.sharedMesh);
            }

            DestroyImmediate(gameObject);
        }

        private void DrawEdgeLines(Vector3[] vertices, int[] faceIndices) {
            GameObject linesContainer = GameObject.Find("Generated Lines");
            if (linesContainer != null) {
                DestroyImmediate(linesContainer);
            }

            linesContainer = new GameObject("Generated Lines");

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
            LogDllWarning();
            UpdateMesh();
        }

        private void LogDllWarning() {
            StringBuilder warning = new StringBuilder(1024);
            getLastWarning(warning, warning.Capacity);
            if (warning.Length > 0) {
                Debug.LogWarning(warning.ToString());
            }
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
                string defaultPath = GetGrammarDataDirectory();
                string path = EditorUtility.OpenFilePanel("Load Grammar", defaultPath, "json");
                if (!string.IsNullOrEmpty(path)) {
                    maxIteration = 0;
                    LoadGrammarFile(path);
                }
            }
            
            if (GUILayout.Button("Load Folder")) {
                string defaultPath = GetGrammarDataDirectory();
                string folderPath = EditorUtility.OpenFolderPanel("Load Grammar Folder", defaultPath, "");
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
                LogDllWarning();
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
        
        private string GetGrammarDataDirectory() {
            // Try to find the grammar data directory relative to the project
            string projectPath = Application.dataPath;
            string grammarDataPath = Path.Combine(projectPath, "..", "grammar data");
            
            // If the grammar data folder exists in the project root, use it
            if (Directory.Exists(grammarDataPath)) {
                return grammarDataPath;
            }
            
            // Fallback: try to find it in common locations
            string[] possiblePaths = {
                Path.Combine(projectPath, "..", "grammar data"),
                Path.Combine(projectPath, "..", "..", "grammar data"),
                Path.Combine(projectPath, "grammar data"),
                Path.Combine(projectPath, "..", "..", "..", "grammar data")
            };
            
            foreach (string path in possiblePaths) {
                if (Directory.Exists(path)) {
                    return path;
                }
            }
            
            // If not found, return the project root
            return Path.GetDirectoryName(projectPath);
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
