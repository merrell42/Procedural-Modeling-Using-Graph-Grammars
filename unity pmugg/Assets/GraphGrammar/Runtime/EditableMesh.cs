using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Profiling;
using UnityEngine.Serialization;
using UnityEngine.Tilemaps;

namespace Grammar {
    [System.Serializable]
    public class FaceType {
        public Vector3 normal;
        // The face set.
        public FaceSet set;
        // Identifies the side within the face set.
        public int sideNum;
        public static int idCounter = 0;
        public int id = 0;
        public string signature;
        public FaceType(Vector3 normal_, FaceSet set_, int sideNum_) {
            normal = normal_;
            sideNum = sideNum_;
            set = set_;
            id = idCounter++;
            signature = set.id + "," + sideNum;
        }

        public bool isFlat() {
            return normal.y == 1 || normal.y == -1;
        }

        public bool Equals(FaceType type) {
            return type.set == set && type.sideNum == sideNum;
        }

        public FaceType Next() {
            var sides = isFlat() ? 1 : set.sides;
            var nextNormal = set.GetNormal((sideNum + 1) % sides);
            return new FaceType(nextNormal, set, (sideNum + 1) % sides);
        }
        public string Signature() {
            return signature;
        }
        public string MirrorImage() {
            var sides = set.sides;
            if (sides % 2 == 1) {
                return "--";
            }
            var mirrored = set.id + "," + ((sideNum + sides / 2) % sides);
            return mirrored;
        }
        public string Export(int index) {
            // color = '[126,132,126]'
            return "<fDecoration types='[" + id + "]'><assignMaterial index='" + index + "' /></fDecoration>";
        }
    }

    [System.Serializable]
    public class ExportFaceDatum {
        public string type;
        public bool onRight;
        public ExportFaceDatum(string type_, bool onRight_) {
            type = type_;
            onRight = onRight_;
        }
     }

    // Connection between a face and an edge.
    [System.Serializable]
    public class FaceDatum {
        public FaceType type;
        public bool onRight;
        public FaceDatum(FaceType type_, bool onRight_) {
            type = type_;
            onRight = onRight_;
        }
        public FaceDatum Next() {
            return new FaceDatum(type.Next(), onRight);
        }
        public bool Equals(FaceDatum datumB) {
            return type.Equals(datumB.type) && onRight == datumB.onRight;
        }
        public string Signature()
        {
            return (onRight ? "R" : "L") + type.Signature();
        }
        public string MirrorImage() {
            return (onRight ? "L" : "R") + type.MirrorImage();
        }
        // Flip just the left and right part.
        public string Flipped() {
            return (onRight ? "L" : "R") + type.Signature();
        }
        public ExportFaceDatum Export() {
            return new ExportFaceDatum(type.signature, onRight);
        }
    }

    [System.Serializable]
    public class ExportEdgeType {
        public ExportFaceDatum[] faceData;
        public Vector3 dir;
        public bool convex;
        public string id;
        public int idNum;

        public ExportEdgeType(ExportFaceDatum[] faceData_, Vector3 dir_, bool convex_, string id_, int idNum_) {
            faceData = faceData_;
            dir = dir_;
            convex = convex_;
            id = id_;
            idNum = idNum_;
        }
    }

    [System.Serializable]
    public class EdgeType {
        public FaceDatum[] faceData;
        public Vector3 dir;
        public bool convex;
        public string id;
        // public static int idCounter = 0;

        public EdgeType(FaceType rightFace, FaceType leftFace, bool convex_) {
            faceData = new FaceDatum[2];
            var nR = rightFace.normal;
            var nL = leftFace.normal;
            faceData[0] = new FaceDatum(rightFace, true);
            faceData[1] = new FaceDatum(leftFace, false);
            convex = convex_;
            dir = (convex ? Vector3.Cross(nL, nR) : Vector3.Cross(nR, nL)).normalized;
            id = Signature();
            // id = idCounter++;
        }

        public EdgeType(FaceDatum[] faceData_, Vector3 dir_, bool convex_) {
            faceData = faceData_;
            dir = dir_;
            convex = convex_;
            id = Signature();
            // id = idCounter++;
        }

        public EdgeType Next() {
            var newFaceData = new FaceDatum[faceData.Length];
            var numSides = 1;
            for (var i = 0; i < faceData.Length; i++) {
                newFaceData[i] = faceData[i].Next();
                numSides = Math.Max(faceData[i].type.set.sides, numSides);
            }
            var theta = 1.0f / numSides * 360;
            var nextDir = FaceSet.RotateAroundYAxis(dir, theta);

            // The direction should be rotated, but it's not.
            return new EdgeType(newFaceData, nextDir, convex);
        }

        // This assumes the faceDatum are in the same order.
        public bool Equals(EdgeType edgeTypeB) {
            return id == edgeTypeB.id;
            /* for (var i = 0; i < faceData.Length; i++) {
                var datumA = faceData[i];
                var datumB = edgeTypeB.faceData[i];
                if (!datumA.Equals(datumB)) {
                    return false;
                }
            }
            return true; */
        }

        public bool IsMirrored() {
            var n = faceData.Length;
            if (n == 2 && faceData[0].MirrorImage().CompareTo(faceData[1].Signature()) == 0) {
                var sign = SignatureBase(false);
                var flipped = SignatureBase(true);
                return sign.CompareTo(flipped) > 0;
            }
            return false;
        }

        public string SignatureBase(bool flipped) {
            var n = faceData.Length;
            var faceSigns = new List<string>();
            for (var i = 0; i < n; i++) {
                faceSigns.Add(flipped ? faceData[i].Flipped() : faceData[i].Signature());
            }
            faceSigns.Sort();
            return (convex ? "T" : "F") + " " + string.Join("|", faceSigns.ToArray());
        }

        public string Signature() {
            return SignatureBase(IsMirrored());
        }

        public ExportEdgeType Export(EdgeMapper edgeMapper) {
            var n = faceData.Length;
            var exportFaceData = new ExportFaceDatum[n];
            // TODO: Save mirrored instead of recalculating it everywhere.
            // var mirrored = IsMirrored();
            for (var i = 0; i < n; i++) {
                // var mi = mirrored ? n - 1 - i : i;
                // exportFaceData[mi] = faceData[mi].Export(mirrored);
                exportFaceData[i] = faceData[i].Export();
            }
            return new ExportEdgeType(exportFaceData, dir, convex, id, edgeMapper.GetId(id));
        }
    }

    [System.Serializable]
    public class ExportVertexConnection {
        public string edge;
        public bool isAtStart;
        public int[] faceIds;

        public ExportVertexConnection(string edge_, bool isAtStart_, int[] faceIds_) {
            edge = edge_;
            isAtStart = isAtStart_;
            faceIds = faceIds_;
        }
    }

    // Connection between an edge and a vertex.
    [System.Serializable]
    public class VertexConnection {
        public EdgeType edge;
        public bool isAtStart;
        public bool isAtStart0;
        public int[] faceIds;
        public VertexConnection(EdgeType edge_, bool isAtStart_, int faceId0, int faceId1) {
            edge = edge_;
            var mirrored = edge.IsMirrored();
            isAtStart0 = isAtStart_;
            isAtStart = isAtStart_ ^ mirrored;
            faceIds = new int[2];
            faceIds[0] = faceId0;
            faceIds[1] = faceId1;
        }
        public bool Equals(VertexConnection connectionB) {
            return edge.Equals(connectionB.edge) && isAtStart == connectionB.isAtStart;
        }
        public VertexConnection Next() {
            return new VertexConnection(edge.Next(), isAtStart0, faceIds[0], faceIds[1]);
        }

        public ExportVertexConnection Export() {
            var mirrored = edge.IsMirrored();
            var exportIds = new int[faceIds.Length];
            if (faceIds.Length == 2) {
                if (!mirrored) {
                    exportIds[0] = faceIds[0];
                    exportIds[1] = faceIds[1];
                } else {
                    exportIds[0] = faceIds[1];
                    exportIds[1] = faceIds[0];
                }
            }
            return new ExportVertexConnection(edge.Signature(), isAtStart, exportIds);
        }
    }

    [System.Serializable]
    public class ExportVertexType {
        public ExportVertexConnection[] connections;
        public int id;

        public ExportVertexType(ExportVertexConnection[] connections_, int id_) {
            connections = connections_;
            id = id_;
        }
    }

    [System.Serializable]
    public class VertexType {
        public VertexConnection[] connections;
        public int id;
        public static int idCounter = 0;

        public VertexType(VertexConnection[] connections_) {
            connections = connections_;
            id = idCounter++;
        }

        // Create a vertex type where all the connections are rotated by 1 side.
        public VertexType Next() {
            var newConnections = new VertexConnection[connections.Length];
            for (var i = 0; i < connections.Length; i++) {
                newConnections[i] = connections[i].Next();
            }
            return new VertexType(newConnections);
        }

        // Tell if two vertex types are the same. The connections can be in a different order.
        public bool Equals(VertexType type) {
            if (connections.Length != type.connections.Length) {
                return false;
            }
            for (var i = 0; i < connections.Length; i++) {
                var success = true;
                for (var j = 0; j < connections.Length && success; j++) {
                    var connectionA = connections[j];
                    var connectionB = type.connections[(i + j) % connections.Length];
                    if (!connectionA.Equals(connectionB)) {
                        success = false;
                    }
                }
                if (success) {
                    return true;
                }
            }
            return false;
        }

        // Consolidate the edge types so matching edges have the same ID.
        /* public void ConsolidateEdges(Dictionary<string, EdgeType> edgeDict) {
            for (var i = 0; i < connections.Length; i++) {
                var sign = connections[i].edge.Signature();
                if (edgeDict.ContainsKey(sign)) {
                    connections[i].edge = edgeDict[sign];
                } else {
                    edgeDict[sign] = connections[i].edge;
                }
            }
        } */

        public ExportVertexType Export() {
            var n = connections.Length;
            var exportConnections = new ExportVertexConnection[n];
            for (var i = 0; i < n; i++) {
                exportConnections[i] = connections[i].Export();
            }
            return new ExportVertexType(exportConnections, id);
        }
    }

    [System.Serializable]
    public class TypeList {
        public List<FaceType> faceTypes;
        public List<ExportEdgeType> edgeTypes;
        public List<ExportVertexType> vertexTypes;
        public bool excludeRepeats;
        public string xml;
        public TypeList() {
            faceTypes = new List<FaceType>();
            edgeTypes = new List<ExportEdgeType>();
            vertexTypes = new List<ExportVertexType>();
            xml = "";
            excludeRepeats = false;
        }
    }

    public class FaceDirections {
        public Vector3 uA;
        public Vector3 uB;
        public Vector3 vA;
        public Vector3 vB;
        public bool convex;
        public FaceDirections(Vector3 uA_, Vector3 uB_, Vector3 vA_, Vector3 vB_, bool convex_) {
            uA = uA_;
            uB = uB_;
            vA = vA_;
            vB = vB_;
            convex = convex_;
        }
    }

    [System.Serializable]
    public class EditableMesh {
        public List<Vector3> vertices;
        public List<Vector2> uv;
        public List<Vector3> normals;
        public List<FaceType> faceTypes;
        public List<int>[] triangleSets;

        public EditableMesh(List<FaceType> faceTypes_) {
            faceTypes = faceTypes_;
            vertices = new List<Vector3>();
            normals = new List<Vector3>();
            uv = new List<Vector2>();
            triangleSets = new List<int>[faceTypes.Count + 2];
            for (var i = 0; i < faceTypes.Count + 2; i++) {
                triangleSets[i] = new List<int>();
            }
        }

        public void AddQuad(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3, int subMesh) {
            var count = vertices.Count;
            vertices.Add(v0);
            vertices.Add(v1);
            vertices.Add(v2);
            vertices.Add(v3);

            Vector3 normal = Vector3.Cross(v1 - v0, v3 - v0).normalized;
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);
            normals.Add(normal);

            var triangles = triangleSets[subMesh];
            triangles.Add(count);
            triangles.Add(count + 1);
            triangles.Add(count + 2);
            triangles.Add(count + 2);
            triangles.Add(count + 1);
            triangles.Add(count);

            triangles.Add(count);
            triangles.Add(count + 2);
            triangles.Add(count + 3);
            triangles.Add(count + 3);
            triangles.Add(count + 2);
            triangles.Add(count);
        }

        public void AddQuadPrism(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 dz, int subMesh0, int subMesh1) {
            // This gives a little space between the main face and the rest to prevent z-fighting.
            var ds = Vector3.zero; // dz * 0.1f;
            AddQuad(v0, v1, v2, v3, subMesh0);
            AddQuad(v0 + dz, v1 + dz, v2 + dz, v3 + dz, subMesh1);
            AddQuad(v0 + ds, v1 + ds, v1 + dz, v0 + dz, subMesh1);
            AddQuad(v1 + ds, v2 + ds, v2 + dz, v1 + dz, subMesh1);
            AddQuad(v2 + ds, v3 + ds, v3 + dz, v2 + dz, subMesh1);
            AddQuad(v3 + ds, v0 + ds, v0 + dz, v3 + dz, subMesh1);
        }

        public void AddBox(Vector3 c, Vector3 dx, Vector3 dy, Vector3 dz, int subMesh0, int subMesh1) {
            AddQuadPrism(c, c + dx, c + dx + dy, c + dy, dz, subMesh0, subMesh1);
        }

        public void AddEdgeType(EdgeType type) {
            for (var i = 0; i < type.faceData.Length; i++) {
                var datum = type.faceData[i];
                var subMesh = faceTypes.IndexOf(datum.type) + 2;
                var u = type.dir;
                var n = datum.type.normal;
                var v = Vector3.Cross(u, n) * (datum.onRight ? 1 : -1);
                AddBox(Vector3.zero, u, v, -0.1f * n, subMesh, 0);
            }
        }

        public static FaceDirections GetFaceDirections(VertexConnection connectionA, VertexConnection connectionB, FaceType type) {
            var n = type.normal;
            var edgeA = connectionA.edge;
            var edgeB = connectionB.edge;
            var startA = connectionA.isAtStart;
            var startB = connectionB.isAtStart;
            var datumA = Array.Find(edgeA.faceData, query => (type.set == query.type.set && type.sideNum == query.type.sideNum));
            var datumB = Array.Find(edgeB.faceData, query => (type.set == query.type.set && type.sideNum == query.type.sideNum));

            var uA = edgeA.dir * (startA ? 1 : -1);
            var uB = edgeB.dir * (startB ? 1 : -1);
            var vA = Vector3.Cross(uA, n) * (datumA.onRight ^ (!startA) ? 1 : -1);
            var vB = Vector3.Cross(uB, n) * (datumB.onRight ^ (!startB) ? 1 : -1);
            var convex = (Vector3.Dot(uA, vB) >= 0);
            return new FaceDirections(uA, uB, vA, vB, convex);
        }

        public void AddVertexType(VertexType type) {
            for (var i = 0; i < type.connections.Length; i++) {
                var connectionA = type.connections[i];
                var edgeA = connectionA.edge;
                foreach (var datumA in edgeA.faceData) {
                    var typeA = datumA.type;
                    // Should we check every connection or just the next one?
                    // for (var j = i + 1; j < type.connections.Length; j++) {

                    var connectionB = type.connections[(i + 1) % type.connections.Length];
                    var edgeB = connectionB.edge;
                    foreach (var datumB in edgeB.faceData) {
                        var typeB = datumB.type;
                        if (typeA.set == typeB.set && typeA.sideNum == typeB.sideNum) {
                            var subMesh = faceTypes.FindIndex(0, query => (typeA.set == query.set && typeA.sideNum == query.sideNum)) + 2;

                            var n = datumA.type.normal;
                            var faceDirections = GetFaceDirections(connectionA, connectionB, typeA);
                            var uA = faceDirections.uA;
                            var uB = faceDirections.uB;
                            var vA = faceDirections.vA;
                            var vB = faceDirections.vB;
                            if (faceDirections.convex) {
                                // The face is convex.
                                AddQuadPrism(Vector3.zero, uA, uA + uB, uB, -0.1f * n, subMesh, 0);
                            } else {
                                // The face is concave.
                                AddQuadPrism(uA, uA - uB, -uA - uB, -uA, -0.1f * n, subMesh, 0);
                                AddQuadPrism(Vector3.zero, uB, uB - uA, -uA, -0.1f * n, subMesh, 0);
                            }
                        }
                    }
                    // }
                }
            }
        }

        public void AddRing(int numVertices, float radius, float length, float theta, int subMesh) {
            float angleDelta = 2 * Mathf.PI / numVertices;
            for (int i = 0; i < numVertices; i++) {
                float angle0 = (i + 0.5f) * angleDelta;
                float angle1 = (i + 1.5f) * angleDelta;
                var u0 = new Vector3(Mathf.Sin(angle0), 0, Mathf.Cos(angle0));
                var u1 = new Vector3(Mathf.Sin(angle1), 0, Mathf.Cos(angle1));
                var v0 = -Mathf.Cos(theta) * u0 + new Vector3(0, Mathf.Sin(theta));
                var v1 = -Mathf.Cos(theta) * u1 + new Vector3(0, Mathf.Sin(theta));
                AddQuad(
                    u0 * radius,
                    u1 * radius,
                    u1 * radius + length * v1,
                    u0 * radius + length * v0,
                    subMesh
                );
            }
        }

        public static Bounds CalculateBoundingBox(List<Vector3> points) {
            if (points == null || points.Count == 0) {
                // Return an invalid bounding box if the list is empty
                return new Bounds(Vector3.zero, Vector3.zero);
            }

            // Initialize min and max vectors with the first point
            Vector3 min = points[0];
            Vector3 max = points[0];

            // Iterate through the list to find the min and max coordinates
            for (int i = 1; i < points.Count; i++) {
                min = Vector3.Min(min, points[i]);
                max = Vector3.Max(max, points[i]);
            }

            // Calculate the center and size of the bounding box
            Vector3 center = (min + max) * 0.5f;
            Vector3 size = max - min;

            // Create and return the bounding box
            Bounds boundingBox = new Bounds(center, size);
            return boundingBox;
        }

        public void CenterVertices(Vector3 newCenter) {
            var box = CalculateBoundingBox(vertices);
            var delta = newCenter - box.center;
            for (var i = 0; i < vertices.Count; i++) {
                vertices[i] += delta;
            }
        }


        public Mesh CreateMesh() {
            var mesh = new Mesh();
            mesh.vertices = vertices.ToArray();
            mesh.uv = uv.ToArray();
            mesh.normals = normals.ToArray();
            mesh.subMeshCount = triangleSets.Length;
            for (var i = 0; i < triangleSets.Length; i++) {
                mesh.SetTriangles(triangleSets[i].ToArray(), i);
            }
            return mesh;
        }
    }

    /// <summary>
    /// Tri/Tri intersection. Implementation of Tomas Moller, 1997.
    /// See article "A Fast Triangle-Triangle Intersection Test", Journal of Graphics Tools, 2(2), 1997.
    /// </summary>
    public static class TriTriOverlap {
        private static void Sort(ref Vector2 v) {
            if (v.x > v.y) {
                float c;
                c = v.x;
                v.x = v.y;
                v.y = c;
            }
        }

        /// <summary>
        /// This edge to edge test is based on Franlin Antonio's gem: "Faster Line Segment Intersection", in Graphics Gems III, pp. 199-202 
        /// </summary>
        private static bool EdgeEdgeTest(Vector3 v0, Vector3 v1, Vector3 u0, Vector3 u1, int i0, int i1) {
            float Ax, Ay, Bx, By, Cx, Cy, e, d, f;
            Ax = v1[i0] - v0[i0];
            Ay = v1[i1] - v0[i1];

            Bx = u0[i0] - u1[i0];
            By = u0[i1] - u1[i1];
            Cx = v0[i0] - u0[i0];
            Cy = v0[i1] - u0[i1];
            f = Ay * Bx - Ax * By;
            d = By * Cx - Bx * Cy;
            if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f)) {
                e = Ax * Cy - Ay * Cx;
                if (f > 0) {
                    if (e >= 0 && e <= f) { return true; }
                } else {
                    if (e <= 0 && e >= f) { return true; }
                }
            }

            return false;
        }

        private static bool EdgeAgainstTriEdges(Vector3 v0, Vector3 v1, Vector3 u0, Vector3 u1, Vector3 u2, short i0, short i1) {
            // test edge u0,u1 against v0,v1
            if (EdgeEdgeTest(v0, v1, u0, u1, i0, i1)) { return true; }

            // test edge u1,u2 against v0,v1 
            if (EdgeEdgeTest(v0, v1, u1, u2, i0, i1)) { return true; }

            // test edge u2,u1 against v0,v1 
            if (EdgeEdgeTest(v0, v1, u2, u0, i0, i1)) { return true; }

            return false;
        }

        private static bool PointInTri(Vector3 v0, Vector3 u0, Vector3 u1, Vector3 u2, short i0, short i1) {
            float a, b, c, d0, d1, d2;

            // is T1 completly inside T2?
            // check if v0 is inside tri(u0,u1,u2)
            a = u1[i1] - u0[i1];
            b = -(u1[i0] - u0[i0]);
            c = -a * u0[i0] - b * u0[i1];
            d0 = a * v0[i0] + b * v0[i1] + c;

            a = u2[i1] - u1[i1];
            b = -(u2[i0] - u1[i0]);
            c = -a * u1[i0] - b * u1[i1];
            d1 = a * v0[i0] + b * v0[i1] + c;

            a = u0[i1] - u2[i1];
            b = -(u0[i0] - u2[i0]);
            c = -a * u2[i0] - b * u2[i1];
            d2 = a * v0[i0] + b * v0[i1] + c;

            if (d0 * d1 > 0.0f) {
                if (d0 * d2 > 0.0f) { return true; }
            }

            return false;
        }

        private static bool TriTriCoplanar(Vector3 N, Vector3 v0, Vector3 v1, Vector3 v2, Vector3 u0, Vector3 u1, Vector3 u2) {
            float[] A = new float[3];
            short i0, i1;

            // first project onto an axis-aligned plane, that maximizes the area
            // of the triangles, compute indices: i0,i1. 
            A[0] = Mathf.Abs(N[0]);
            A[1] = Mathf.Abs(N[1]);
            A[2] = Mathf.Abs(N[2]);
            if (A[0] > A[1]) {
                if (A[0] > A[2]) {
                    // A[0] is greatest
                    i0 = 1;
                    i1 = 2;
                } else {
                    // A[2] is greatest
                    i0 = 0;
                    i1 = 1;
                }
            } else {
                if (A[2] > A[1]) {
                    // A[2] is greatest 
                    i0 = 0;
                    i1 = 1;
                } else {
                    // A[1] is greatest 
                    i0 = 0;
                    i1 = 2;
                }
            }

            // test all edges of triangle 1 against the edges of triangle 2 
            if (EdgeAgainstTriEdges(v0, v1, u0, u1, u2, i0, i1)) { return true; }
            if (EdgeAgainstTriEdges(v1, v2, u0, u1, u2, i0, i1)) { return true; }
            if (EdgeAgainstTriEdges(v2, v0, u0, u1, u2, i0, i1)) { return true; }

            // finally, test if tri1 is totally contained in tri2 or vice versa 
            if (PointInTri(v0, u0, u1, u2, i0, i1)) { return true; }
            if (PointInTri(u0, v0, v1, v2, i0, i1)) { return true; }

            return false;
        }



        private static bool ComputeIntervals(float VV0, float VV1, float VV2,
                                  float D0, float D1, float D2, float D0D1, float D0D2,
                                  ref float A, ref float B, ref float C, ref float X0, ref float X1) {
            if (D0D1 > 0.0f) {
                // here we know that D0D2<=0.0 
                // that is D0, D1 are on the same side, D2 on the other or on the plane 
                A = VV2; B = (VV0 - VV2) * D2; C = (VV1 - VV2) * D2; X0 = D2 - D0; X1 = D2 - D1;
            } else if (D0D2 > 0.0f) {
                // here we know that d0d1<=0.0 
                A = VV1; B = (VV0 - VV1) * D1; C = (VV2 - VV1) * D1; X0 = D1 - D0; X1 = D1 - D2;
            } else if (D1 * D2 > 0.0f || D0 != 0.0f) {
                // here we know that d0d1<=0.0 or that D0!=0.0 
                A = VV0; B = (VV1 - VV0) * D0; C = (VV2 - VV0) * D0; X0 = D0 - D1; X1 = D0 - D2;
            } else if (D1 != 0.0f) {
                A = VV1; B = (VV0 - VV1) * D1; C = (VV2 - VV1) * D1; X0 = D1 - D0; X1 = D1 - D2;
            } else if (D2 != 0.0f) {
                A = VV2; B = (VV0 - VV2) * D2; C = (VV1 - VV2) * D2; X0 = D2 - D0; X1 = D2 - D1;
            } else {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Checks if the triangle V(v0, v1, v2) intersects the triangle U(u0, u1, u2).
        /// </summary>
        /// <param name="v0">Vertex 0 of V</param>
        /// <param name="v1">Vertex 1 of V</param>
        /// <param name="v2">Vertex 2 of V</param>
        /// <param name="u0">Vertex 0 of U</param>
        /// <param name="u1">Vertex 1 of U</param>
        /// <param name="u2">Vertex 2 of U</param>
        /// <returns>Returns <c>true</c> if V intersects U, otherwise <c>false</c></returns>
        public static bool TriTriIntersect(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 u0, Vector3 u1, Vector3 u2) {
            Vector3 e1, e2;
            Vector3 n1, n2;
            Vector3 dd;
            Vector2 isect1 = Vector2.zero, isect2 = Vector2.zero;

            float du0, du1, du2, dv0, dv1, dv2, d1, d2;
            float du0du1, du0du2, dv0dv1, dv0dv2;
            float vp0, vp1, vp2;
            float up0, up1, up2;
            float bb, cc, max;

            short index;

            // compute plane equation of triangle(v0,v1,v2) 
            e1 = v1 - v0;
            e2 = v2 - v0;
            n1 = Vector3.Cross(e1, e2);
            d1 = -Vector3.Dot(n1, v0);
            // plane equation 1: N1.X+d1=0 */

            // put u0,u1,u2 into plane equation 1 to compute signed distances to the plane
            du0 = Vector3.Dot(n1, u0) + d1;
            du1 = Vector3.Dot(n1, u1) + d1;
            du2 = Vector3.Dot(n1, u2) + d1;

            // coplanarity robustness check 
            if (Mathf.Abs(du0) < Mathf.Epsilon) { du0 = 0.0f; }
            if (Mathf.Abs(du1) < Mathf.Epsilon) { du1 = 0.0f; }
            if (Mathf.Abs(du2) < Mathf.Epsilon) { du2 = 0.0f; }

            du0du1 = du0 * du1;
            du0du2 = du0 * du2;

            // same sign on all of them + not equal 0 ? 
            if (du0du1 > 0.0f && du0du2 > 0.0f) {
                // no intersection occurs
                return false;
            }

            // compute plane of triangle (u0,u1,u2)
            e1 = u1 - u0;
            e2 = u2 - u0;
            n2 = Vector3.Cross(e1, e2);
            d2 = -Vector3.Dot(n2, u0);

            // plane equation 2: N2.X+d2=0 
            // put v0,v1,v2 into plane equation 2
            dv0 = Vector3.Dot(n2, v0) + d2;
            dv1 = Vector3.Dot(n2, v1) + d2;
            dv2 = Vector3.Dot(n2, v2) + d2;

            if (Mathf.Abs(dv0) < Mathf.Epsilon) { dv0 = 0.0f; }
            if (Mathf.Abs(dv1) < Mathf.Epsilon) { dv1 = 0.0f; }
            if (Mathf.Abs(dv2) < Mathf.Epsilon) { dv2 = 0.0f; }


            dv0dv1 = dv0 * dv1;
            dv0dv2 = dv0 * dv2;

            // same sign on all of them + not equal 0 ? 
            if (dv0dv1 > 0.0f && dv0dv2 > 0.0f) {
                // no intersection occurs
                return false;
            }

            // compute direction of intersection line 
            dd = Vector3.Cross(n1, n2);

            // compute and index to the largest component of D 
            max = (float)Mathf.Abs(dd[0]);
            index = 0;
            bb = (float)Mathf.Abs(dd[1]);
            cc = (float)Mathf.Abs(dd[2]);
            if (bb > max) { max = bb; index = 1; }
            if (cc > max) { max = cc; index = 2; }

            // this is the simplified projection onto L
            vp0 = v0[index];
            vp1 = v1[index];
            vp2 = v2[index];

            up0 = u0[index];
            up1 = u1[index];
            up2 = u2[index];

            // compute interval for triangle 1 
            float a = 0, b = 0, c = 0, x0 = 0, x1 = 0;
            if (ComputeIntervals(vp0, vp1, vp2, dv0, dv1, dv2, dv0dv1, dv0dv2, ref a, ref b, ref c, ref x0, ref x1)) {
                return TriTriCoplanar(n1, v0, v1, v2, u0, u1, u2);
            }

            // compute interval for triangle 2 
            float d = 0, e = 0, f = 0, y0 = 0, y1 = 0;
            if (ComputeIntervals(up0, up1, up2, du0, du1, du2, du0du1, du0du2, ref d, ref e, ref f, ref y0, ref y1)) {
                return TriTriCoplanar(n1, v0, v1, v2, u0, u1, u2);
            }

            float xx, yy, xxyy, tmp;
            xx = x0 * x1;
            yy = y0 * y1;
            xxyy = xx * yy;

            tmp = a * xxyy;
            isect1[0] = tmp + b * x1 * yy;
            isect1[1] = tmp + c * x0 * yy;

            tmp = d * xxyy;
            isect2[0] = tmp + e * xx * y1;
            isect2[1] = tmp + f * xx * y0;

            Sort(ref isect1);
            Sort(ref isect2);

            return !(isect1[1] < isect2[0] || isect2[1] < isect1[0]);
        }
    }
}