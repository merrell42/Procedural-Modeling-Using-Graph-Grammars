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
        [MenuItem("Window/Grammar Editor")]
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
        const string dll = "RuleGenerator.dll";
        const string pmuggDll = "pmugg dll.dll";
#endif
        // [DllImport(dll)]
        // private static extern string ExampleStringFunction();
        [DllImport(dll)]
        private static extern float ExampleFloatFunction();
        [DllImport(dll)]
        private static extern IntPtr ExampleHelloWorld();
        [DllImport(dll)]
        private static extern float ExampleAdd(float a, float b);
        [DllImport(dll, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GenerateRules(string input, StringBuilder str, int len);
        
        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern int SayHello();

        [StructLayout(LayoutKind.Sequential)]
        public struct Vec3DLL {
            public float x;
            public float y;
            public float z;
        }

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void DoubleVector();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void ResetVector();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern Vec3DLL GetTestVector();


        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern int initialize();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void reset();

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern void iterate(int steps);

        [DllImport(pmuggDll, CallingConvention = CallingConvention.Cdecl)]
        private static extern int getNumFaces();

        private void OnEnable() {
            titleContent = new GUIContent("Grammar Editor");
        }

        private const string faceGroupName = "Faces";
        private const string edgeGroupName = "Edges";
        private bool faceOptionsToggle = true;

        public static GameObject CreateGameObject(EditableMesh editableMesh) {
            GameObject gameObject = new GameObject();

            // Add required components to display a mesh.
            MeshFilter meshFilter = gameObject.AddComponent<MeshFilter>();
            MeshRenderer meshRenderer = gameObject.AddComponent<MeshRenderer>();
            var material = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Material.mat");
            meshRenderer.material = material;
            meshFilter.mesh = editableMesh.CreateMesh();
            return gameObject;
        }

        void OnGUI() {
            // GUILayout.Label("View", EditorStyles.boldLabel);

            var r = EditorGUILayout.BeginVertical();

            faceOptionsToggle = EditorGUILayout.BeginFoldoutHeaderGroup(faceOptionsToggle, "Create Face Set");
            if (faceOptionsToggle) {
                GrammarEditorState.faceName = EditorGUILayout.TextField(
                    new GUIContent("Name", "The name of the new face set."),
                    GrammarEditorState.faceName);
                GrammarEditorState.sides = EditorGUILayout.IntField(
                    new GUIContent("Sides", "The number of sides of new face set."),
                    GrammarEditorState.sides);
                GrammarEditorState.angle = EditorGUILayout.FloatField(
                    new GUIContent("Angle", "The angle with respect to the ground. 0 is parallel to the ground. 90 is vertical."),
                    GrammarEditorState.angle);
            }

            EditorGUILayout.EndFoldoutHeaderGroup();

            if (GUILayout.Button("Create Polygon")) {
                var theta = GrammarEditorState.angle / 180 * Mathf.PI;
                int numVertices = GrammarEditorState.sides;
                float radius = 5;

                var faceTypes = new List<FaceType>();
                var editableMesh = new EditableMesh(faceTypes);
                editableMesh.AddRing(numVertices, radius, 1, theta, 0);

                var gameObject = CreateGameObject(editableMesh);
                gameObject.name = GrammarEditorState.faceName;
                FaceSet faceSet = gameObject.AddComponent<FaceSet>();
                faceSet.angle = GrammarEditorState.angle;
                faceSet.sides = faceSet.isFlat() ? 1 : GrammarEditorState.sides;
                // faceSet.id = IdCounter.faceSetCounter++;

                faceSet.types = new List<FaceType>();
                for (var i = 0; i < faceSet.sides; i++) {
                    faceSet.types.Add(new FaceType(faceSet.GetNormal(i), faceSet, i));
                }

                Selection.activeGameObject = gameObject;
                // UnityEngineInterface.Instance.Destroy(gameObject);

                var creator = FindObjectOfType<GrammarCreator>();
                if (creator) {
                    gameObject.transform.parent = creator.transform.Find(faceGroupName);
                } else {
                    Debug.LogWarning("GrammarCreator not found");
                }
            }
            EditorGUILayout.EndFoldoutHeaderGroup();

            if (GUILayout.Button("Create Edge")) {
                var faceSets = Array.FindAll(Selection.gameObjects, obj => {
                    if (obj.GetComponent<FaceSet>()) {
                        return true;
                    } else {
                        Debug.LogWarning("One of the selected objects is not a FaceSet.");
                        return false;
                    }
                });
                CreateEdge(faceSets);
            }

            if (GUILayout.Button("Clear Vertices")) {
                var creator = FindObjectOfType<GrammarCreator>();
                if (creator) {
                    var vertexGroup = creator.transform.Find("Vertices");
                    for (int i = vertexGroup.transform.childCount; i > 0; --i) {
                        DestroyImmediate(vertexGroup.transform.GetChild(0).gameObject);
                    }
                }
            }

            if (GUILayout.Button("Create Vertices")) {
                var selectedEdges = new List<EdgeTypeComponent>();
                Array.ForEach(Selection.gameObjects, obj => {
                    var component = obj.GetComponent<EdgeTypeComponent>();
                    if (component) {
                        selectedEdges.Add(component);
                    } else {
                        Debug.LogWarning("One of the selected objects is not an edge.");
                    }
                });
                EdgeTypeComponentEditor.AddVertexTypes(selectedEdges.ToArray());
            }

            GrammarEditorState.excludeRepeats = EditorGUILayout.Toggle(
                new GUIContent("Exclude Repeats", "Exclude any rules that use the same vertex types."),
                GrammarEditorState.excludeRepeats);


            if (GUILayout.Button("Initialize")) {
                Debug.Log(initialize());
            }

            if (GUILayout.Button("reset")) {
                reset();
            }

            if (GUILayout.Button("iterate")) {
                iterate(1);
            }

            if (GUILayout.Button("faces")) {
                Debug.Log(getNumFaces());
            }

            if (GUILayout.Button("Generate")) {
                DoubleVector();
                Vec3DLL v = GetTestVector();
                Debug.Log($"A: {v.x}");
            }

            if (GUILayout.Button("Generate Rules")) {
                var edgeMapper = new EdgeMapper();
                var fComponents = FindObjectsOfType<FaceSet>();
                var eComponents = FindObjectsOfType<EdgeTypeComponent>();
                var vComponents = FindObjectsOfType<VertexTypeComponent>();
                var typeList = new TypeList();
                var decoration = "<decoration>";
                var faceIndex = 1;
                for (var i = 0; i < fComponents.Length; i++) {
                    var f = fComponents[i];
                    typeList.faceTypes.AddRange(f.types);
                    for (var j = 0; j < f.types.Count; j++) {
                        decoration += f.types[j].Export(faceIndex);
                    }
                    faceIndex++;
                }
                for (var i = 0; i < eComponents.Length; i++) {
                    var e = eComponents[i];
                    decoration += e.Export(edgeMapper);
                    // if (e.typeEnabled) {
                        for (var j = 0; j < e.types.Count; j++) {
                            if (!e.types[j].IsMirrored()) {
                                typeList.edgeTypes.Add(e.types[j].Export(edgeMapper));
                            }
                        }
                    // }
                }
                for (var i = 0; i < vComponents.Length; i++) {
                    var v = vComponents[i];
                    decoration += v.Export();
                    if (v.typeEnabled) {
                        for (var j = 0; j < v.types.Count; j++) {
                            typeList.vertexTypes.Add(v.types[j].Export());
                        }
                    }
                }
                decoration += "</decoration>";
                if (typeList.vertexTypes.Count == 0) {
                    Debug.Log("No Vertex Types Enabled");
                } else {
                    typeList.xml = decoration;
                    typeList.excludeRepeats = GrammarEditorState.excludeRepeats;
                    var json = JsonUtility.ToJson(typeList);
                    Debug.Log(json);
                    File.WriteAllText("C:/model synthesis/model_synthesis_files/Grammar Editor/RuleGeneratorTest/RuleGeneratorTest/json.txt", json);

                    StringBuilder sb = new StringBuilder(100000);
                    GenerateRules(json, sb, sb.Capacity);
                    Debug.Log(sb);
                    GUIUtility.systemCopyBuffer = sb.ToString();
                }
            }

            EditorGUILayout.EndVertical();
        }

        private void CreateEdge(GameObject[] gameObjectsArray) {
            var gameObjects = gameObjectsArray.ToList();
            if (gameObjects.Count == 0) {
                Debug.LogWarning("No face sets.");
                return;
            }
            if (gameObjects.Count == 1) {
                gameObjects.Add(gameObjects[0]);
            }
            if (gameObjects.Count > 2) {
                Debug.LogWarning("Not implemented for more than two objects.");
                return;
            }
            var set0 = gameObjects[0].GetComponent<FaceSet>();
            var set1 = gameObjects[1].GetComponent<FaceSet>();
            if (set0.isFlat()) {
                // Don't let the ground be set0.
                var temp = set0;
                set0 = set1;
                set1 = temp;
            }
            // The number of sides after combining symmetric ones.
            var canonicalSides = set0.sides;
            var fullSides = Mathf.Max(set0.sides, set1.sides);
            if (set0.isFlat() || set1.isFlat()) {
                canonicalSides = 1;
            } else {
                if (set0.sides != set1.sides) {
                    Debug.LogWarning("Different number of sides not implemented.");
                }
            }

            var creator = FindObjectOfType<GrammarCreator>();
            GameObject edgeGroup = new GameObject();
            edgeGroup.name = set0.name + " - " + set1.name;
            if (creator) {
                edgeGroup.transform.parent = creator.transform.Find(edgeGroupName);
            } else {
                Debug.LogWarning("GrammarCreator not found");
            }

            var numCombinations = (set0 == set1 ? canonicalSides / 2 + 1 : canonicalSides);
            for (var i = 0; i < numCombinations; i++) {
                var n0 = set0.GetNormal(i);
                var n1 = set1.GetNormal(0);
                if (Vector3.Cross(n0, n1).magnitude < 0.0001) {
                    continue;
                }

                for (var j = 0; j < 2; j++) {
                    var convex = (j == 0);
                    var edgeTypes = new List<EdgeType>();
                    for (var k = 0; k < fullSides; k++) {
                        var index0 = set0.isFlat() ? 0 : (k + i) % fullSides;
                        var index1 = set1.isFlat() ? 0 : k;
                        var faceK0 = set0.types[index0];
                        var faceK1 = set1.types[index1];
                        var edgeType = new EdgeType(faceK0, faceK1, convex);
                        edgeTypes.Add(edgeType);
                    }

                    // Draw just the first edge type.
                    var face0 = set0.types[set0.isFlat() ? 0 : i];
                    var face1 = set1.types[0];
                    var faceTypes = new List<FaceType> { face1, face0 };
                    var mesh = new EditableMesh(faceTypes);
                    mesh.AddEdgeType(edgeTypes[0]);
                    mesh.CenterVertices(new Vector3(0, 0.5f, 0));
                    // mesh.AddRing(24, 1, 0.1f, 0, 1);

                    var obj = CreateGameObject(mesh);
                    var theta0 = (float)i / (float)canonicalSides * 2 * Mathf.PI;
                    obj.name = (convex ? "Convex " : "Concave ") + Mathf.Round(theta0 / Mathf.PI * 180);
                    obj.transform.parent = edgeGroup.transform;
                    obj.transform.position = new Vector3(3 * i, 0, 3 * j);

                    EdgeTypeComponent edgeTypeComponent = obj.AddComponent<EdgeTypeComponent>();
                    edgeTypeComponent.types = edgeTypes;
                    // edgeTypeComponent.typeEnabled = convex;

                    var mat0 = gameObjects[0].GetComponent<MeshRenderer>().sharedMaterial;
                    var mat1 = gameObjects[1].GetComponent<MeshRenderer>().sharedMaterial;
                    var meshRenderer = obj.GetComponent<MeshRenderer>();
                    var defaultMat = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Material.mat");
                    Material enabledMat = Resources.Load<Material>(convex ? "enabled" : "disabled");
                    meshRenderer.materials = new[] { defaultMat, enabledMat, mat0, mat1 };
                }
            }
        }
    }
}
