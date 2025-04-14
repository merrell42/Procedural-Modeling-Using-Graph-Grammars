using UnityEditor;
using UnityEngine;
using System;
using UnityEditorInternal;
using UnityEditor.IMGUI.Controls;
using System.Threading.Tasks;
using System.Threading;
using System.Linq;
using System.Collections.Generic;

namespace Grammar {
    [CustomEditor(typeof(EdgeTypeComponent))]
    public class EdgeTypeComponentEditor : Editor {
        private void OnSceneGUI() {
            /* EdgeTypeComponent component = (EdgeTypeComponent)target;

            Event currentEvent = Event.current;
            if (currentEvent.type == EventType.KeyDown) {
                if (currentEvent.keyCode == KeyCode.Space) {
                    component.typeEnabled = !component.typeEnabled;
                    var obj = component.gameObject;
                    var meshRenderer = obj.GetComponent<MeshRenderer>();
                    Material[] materials = meshRenderer.sharedMaterials.Clone() as Material[];
                    materials[1] = component.typeEnabled ? Resources.Load<Material>("enabled") : Resources.Load<Material>("disabled");
                    meshRenderer.materials = materials;

                    // UpdateVertexTypes();
                }
            } */
        }
        const int numCols = 10;

        public static bool DoTriangleIntersect(List<Vector3> listA, List<Vector3> listB) {
            for (var a = 0; a < listA.Count; a += 3) {
                for (var b = 0; b < listB.Count; b += 3) {
                    if (TriTriOverlap.TriTriIntersect(listA[a], listA[a + 1], listA[a + 2], listB[b], listB[b + 1], listB[b + 2])) {
                        return true;
                    }
                }
            }
            return false;
        }

        public static List<int> MatchFaceIds(VertexConnection prevConnection, VertexConnection newConnection, int newId) {
            var edgeType = newConnection.edge;
            var prevEdgeType = prevConnection.edge;
            List<int> newFaceIds = new List<int>();
            bool hasMatch = false;
            for (var n = 0; n < 2; n++) {
                for (var m = 0; m < 2; m++) {
                    if (edgeType.faceData[n].type.id == prevEdgeType.faceData[m].type.id) {
                        hasMatch = true;
                        newFaceIds.Add(prevConnection.faceIds[m]);
                    }
                }
                if (newFaceIds.Count == n) {
                    newFaceIds.Add(newId);
                }
            }
            if (!hasMatch) {
                Debug.LogWarning("Neither of the previous two face types match.");
            }
            return newFaceIds;
        }

        // Generate the triangles to test for self-intersection.
        public static List<Vector3> GetFaceTriangles(FaceDirections faceDirections) {
            var newTriangles = new List<Vector3>();
            const float epsilon = 0.01f;
            System.Random random = new System.Random();
            var uA = faceDirections.uA; var uB = faceDirections.uB;
            var vA = faceDirections.vA; var vB = faceDirections.vB;
            // A small random space to ensure there are real intersections within the face, not along touching edges.
            var sA = faceDirections.vA * epsilon * (1 + (float)random.NextDouble());
            var sB = faceDirections.vB * epsilon * (1 + (float)random.NextDouble());
            if (faceDirections.convex) {
                newTriangles.Add(sA + sB); newTriangles.Add(sA + uA); newTriangles.Add(sB + uB);
            } else {
                newTriangles.Add(sA + sB); newTriangles.Add(sA + uA); newTriangles.Add(-uB);
                newTriangles.Add(sA + sB); newTriangles.Add(sB + uB); newTriangles.Add(-uA);
                newTriangles.Add(sA + sB); newTriangles.Add(-uA); newTriangles.Add(-uB);
            }
            return newTriangles;
        }

        public static void FindVertexTypes(UpdateState state, List<VertexType> vertexTypes) {
            var prevFace = state.faceTypes.Last();
            var secondPrev = state.faceTypes[state.faceTypes.Count - 2];
            // Accept this state if we do a loop around the vertex and get back to the original face.
            var face0 = state.faceTypes[0];
            if (face0.set == prevFace.set && face0.sideNum == prevFace.sideNum) {
                // Check if the last face intersects the others.
                var faceDirections = EditableMesh.GetFaceDirections(state.connections[0], state.connections.Last(), face0);
                var newTriangles = GetFaceTriangles(faceDirections);
                if (DoTriangleIntersect(state.triangles, newTriangles)) {
                    return;
                }
                var newFaceIds = MatchFaceIds(state.connections[0], state.connections.Last(), -1);
                var lastConnection = state.connections.Last();
                for (var i = 0; i < 2; i++) {
                    if (newFaceIds[i] >= 0 && lastConnection.faceIds[i] == state.connections.Count) {
                        lastConnection.faceIds[i] = newFaceIds[i];
                    }
                    /* if (lastConnection.faceIds[i] == state.connections.Count) {
                        lastConnection.faceIds[i] = 0;
                    } */
                }
                var newType = new VertexType(state.connections.ToArray());

                // Ignore if this is a repeat of another vertexType.
                var vComponents = FindObjectsOfType<VertexTypeComponent>();
                foreach (var vComponent in vComponents) {
                    foreach (var vType in vComponent.types) {
                        if (newType.Equals(vType)) {
                            return;
                        }
                    }
                }

                vertexTypes.Add(newType);

                var mesh = new EditableMesh(state.faceTypes);
                mesh.AddVertexType(newType);

                // Debug.Log(JsonUtility.ToJson(newType));

                mesh.CenterVertices(new Vector3(0, 0.5f, 0));
                mesh.AddRing(24, 1, 0.1f, 0, 1);
                var obj = GrammarEditorWindow.CreateGameObject(mesh);

                var sets = FindObjectsOfType<FaceSet>();

                var meshRenderer = obj.GetComponent<MeshRenderer>();
                var materials = new Material[2 + state.faceTypes.Count];
                materials[0] = AssetDatabase.GetBuiltinExtraResource<Material>("Default-Material.mat");
                materials[1] = Resources.Load<Material>("enabled");
                for (var i = 0; i < state.faceTypes.Count; i++) {
                    var faceType = state.faceTypes[i];
                    materials[i + 2] = Array.Find(sets, set => faceType.set == set).GetComponent<MeshRenderer>().sharedMaterial;
                }
                meshRenderer.materials = materials;

                var creator = FindObjectOfType<GrammarCreator>();
                if (creator) {
                    obj.transform.parent = creator.transform.Find("Vertices");
                } else {
                    Debug.LogWarning("GrammarCreator not found");
                }
                var vCount = vComponents.Length;
                obj.transform.position = new Vector3(3 * (vCount % numCols), 0, 3 * (vCount / numCols));
                VertexTypeComponent vertexTypeComponent = obj.AddComponent<VertexTypeComponent>();
                vertexTypeComponent.types.Add(newType);
                vertexTypeComponent.typeEnabled = true;

                // Continue to add rotated types until they repeat.
                var nextType = newType.Next();
                var count = 0;
                while (!nextType.Equals(newType) && count < 10) {
                    vertexTypeComponent.types.Add(nextType);
                    nextType = nextType.Next();
                    count++;
                }
                /* for (var i = 0; i < vertexTypeComponent.types.Count; i++) {
                    vertexTypeComponent.types[i].ConsolidateEdges(edgeDict);
                } */
                return;
            }

            // As a simple hack limit the number of faces.
            if (state.faceTypes.Count > 5) {
                return;
            }

            for (var i = state.minComponent; i < state.components.Length; i++) {
                var component = state.components[i];
                var edgeTypes = component.types;
                foreach (var edgeType in edgeTypes) {
                    for (var j = 0; j < edgeType.faceData.Length; j++) {
                        var matchingFace = edgeType.faceData[j].type;
                        if (!(matchingFace.set == prevFace.set && matchingFace.sideNum == prevFace.sideNum)) {
                            continue;
                        }
                        var newDatum = edgeType.faceData[1 - j];
                        var newFace = newDatum.type;

                        // The new face cannot have same normal as the second previous face.
                        // This check may not be necessary with the check that the edges aren't parallel.
                        if (Mathf.Abs(Vector3.Dot(secondPrev.normal, newFace.normal)) > 0.999) {
                            continue;
                        }


                        // The vertex types are built going counterclockwise. The matching face
                        // should be on the right and the new face on the left.
                        var newState = state.Clone();
                        newState.faceTypes.Add(newFace);
                        var connectionA = newState.connections.Last();
                        var connectionB = new VertexConnection(edgeType, !newDatum.onRight, 0, 0);
                        var newFaceIds = MatchFaceIds(connectionA, connectionB, newState.connections.Count + 1);
                        connectionB.faceIds[0] = newFaceIds[0];
                        connectionB.faceIds[1] = newFaceIds[1];
                        newState.connections.Add(connectionB);

                        // This assumes there are two faces.
                        /* var idCount = newState.connections.Count;
                        var faceId0 = (j == 0) ? idCount : idCount + 1;
                        var faceId1 = (j == 0) ? idCount + 1: idCount;
                        var connectionB = new VertexConnection(edgeType, !newDatum.onRight, faceId0, faceId1);
                        newState.connections.Add(connectionB); */

                        var faceDirections = EditableMesh.GetFaceDirections(connectionA, connectionB, matchingFace);

                        // Reject if two consecutive edges are parallel.
                        if (Mathf.Abs(Vector3.Dot(faceDirections.uA, faceDirections.uB)) > 0.99) {
                            continue;
                        }

                        var newTriangles = GetFaceTriangles(faceDirections);

                        if (DoTriangleIntersect(newState.triangles, newTriangles)) {
                            continue;
                        }
                        newState.triangles.AddRange(newTriangles);

                        FindVertexTypes(newState, vertexTypes);
                    }
                }
            }
        }

        public static void AddVertexTypes(EdgeTypeComponent[] edgeTypes) {
            // var components = FindObjectsOfType<EdgeTypeComponent>();
            // var enabled = Array.FindAll(components, component => component.typeEnabled);
            List<VertexType> vertexTypes = new List<VertexType>();

            for (var i = 0; i < edgeTypes.Length; i++) {
                var component = edgeTypes[i];
                // Only use the first edge type since the rest are symmetric.
                var edgeType = component.types[0];

                // var state = new UpdateState(enabled, i);
                // This ignores minComponent.
                var state = new UpdateState(edgeTypes, 0);

                state.connections.Add(new VertexConnection(edgeType, true, 0, 1));
                var faceTypes = new List<FaceType>();
                var rightFace = Array.Find(edgeType.faceData, datum => datum.onRight).type;
                var leftFace = Array.Find(edgeType.faceData, datum => !datum.onRight).type;
                state.faceTypes.Add(rightFace);
                state.faceTypes.Add(leftFace);

                FindVertexTypes(state, vertexTypes);
                // edgeTypeA.faceData[0].type;
            }
        }
    }

    public class UpdateState {
        public List<FaceType> faceTypes;
        public List<VertexConnection> connections;
        public List<Vector3> triangles;
        public EdgeTypeComponent[] components;
        // The minimum component that can be used. Components before this are ignored.
        public int minComponent;

        public UpdateState(EdgeTypeComponent[] components_, int minComponent_) {
            components = components_;
            minComponent = minComponent_;
            faceTypes = new List<FaceType>();
            connections = new List<VertexConnection>();
            triangles = new List<Vector3>();
        }

        public UpdateState Clone() {
            var copy = new UpdateState(components, minComponent);
            copy.faceTypes = new List<FaceType>(faceTypes);
            copy.connections = new List<VertexConnection>(connections);
            copy.triangles = new List<Vector3>(triangles);
            return copy;
        }
    }
}