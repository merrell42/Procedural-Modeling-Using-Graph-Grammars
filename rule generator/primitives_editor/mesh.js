// Mesh topology for the primitives editor.
//
// Type indices follow cpp_version/mesh_extraction (half_edge_mesh.cpp and
// type_extractor.cpp), with useGroupAsVolume = false. A primitives JSON does
// not store which face, edge, or vertex uses which type, so a click has to
// recover that assignment from the embedded OBJ.

(function (root, factory) {
    if (typeof module === 'object' && module.exports) {
        module.exports = factory(require('./earcut.js'));
    } else {
        root.PrimitivesMesh = factory(root.earcut);
    }
})(typeof self !== 'undefined' ? self : this, function (earcut) {
    var NORMAL_EPS = 1e-4;
    var DIR_EPS = 1e-4;

    function llround(v) {
        return v >= 0 ? Math.round(v) : -Math.round(-v);
    }

    function quantize(v, eps) {
        return llround(v / eps);
    }

    function qkey(v, eps) {
        return quantize(v.x, eps) + ',' + quantize(v.y, eps) + ',' + quantize(v.z, eps);
    }

    function parseObj(text) {
        var vertices = [];
        var faces = [];
        var material = '';
        var lines = text.split(/\r?\n/);
        for (var li = 0; li < lines.length; li++) {
            var line = lines[li].trim();
            if (!line || line.charAt(0) === '#') continue;
            var parts = line.split(/\s+/);
            var key = parts[0];
            if (key === 'v' && parts.length >= 4) {
                vertices.push({
                    x: parseFloat(parts[1]),
                    y: parseFloat(parts[2]),
                    z: parseFloat(parts[3]),
                });
            } else if (key === 'usemtl' && parts.length >= 2) {
                material = parts.slice(1).join(' ');
            } else if (key === 'f' && parts.length >= 4) {
                var corners = [];
                for (var i = 1; i < parts.length; i++) {
                    var indexToken = parts[i].split('/')[0];
                    if (!indexToken) continue;
                    var index = parseInt(indexToken, 10);
                    if (index > 0) index = index - 1;
                    else if (index < 0) index = vertices.length + index;
                    else continue;
                    corners.push(index);
                }
                if (corners.length < 3) {
                    throw new Error('face ' + faces.length + ' has ' + corners.length + ' corners (< 3)');
                }
                faces.push({ corners: corners, material: material });
            }
        }
        if (!vertices.length || !faces.length) {
            throw new Error('OBJ has no vertices or faces');
        }
        return { vertices: vertices, faces: faces };
    }

    function newellNormal(vertices, corners) {
        var nx = 0, ny = 0, nz = 0;
        var n = corners.length;
        for (var i = 0; i < n; i++) {
            var a = vertices[corners[i]];
            var b = vertices[corners[(i + 1) % n]];
            nx += (a.y - b.y) * (a.z + b.z);
            ny += (a.z - b.z) * (a.x + b.x);
            nz += (a.x - b.x) * (a.y + b.y);
        }
        var len = Math.sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0) {
            nx /= len;
            ny /= len;
            nz /= len;
        }
        return { x: nx, y: ny, z: nz };
    }

    function splitPinchedLoops(corners) {
        var n = corners.length;
        if (n < 3) return [];
        for (var i = 0; i < n; i++) {
            for (var j = i + 1; j < n; j++) {
                if (corners[i] !== corners[j]) continue;
                var loopA = corners.slice(i, j);
                var loopB = corners.slice(j).concat(corners.slice(0, i));
                return splitPinchedLoops(loopA).concat(splitPinchedLoops(loopB));
            }
        }
        return [corners.slice()];
    }

    function directedKey(from, to) {
        return from + '>' + to;
    }

    function buildHalfEdgeMesh(obj) {
        var mesh = {
            halfEdges: [],
            faces: [],
            edges: [],
            vertexHalves: obj.vertices.map(function () { return []; }),
        };

        for (var fi = 0; fi < obj.faces.length; fi++) {
            var src = obj.faces[fi];
            var dst = {
                halfEdges: [],
                normal: newellNormal(obj.vertices, src.corners),
                material: src.material,
                volAbove: 0,
                volBelow: 1,
            };
            var loops = splitPinchedLoops(src.corners);
            for (var li = 0; li < loops.length; li++) {
                var loop = loops[li];
                var nCorners = loop.length;
                var firstHE = mesh.halfEdges.length;
                for (var k = 0; k < nCorners; k++) {
                    var he = {
                        origin: loop[k],
                        dest: loop[(k + 1) % nCorners],
                        face: fi,
                        twin: -1,
                        next: firstHE + (k + 1) % nCorners,
                        prev: firstHE + (k - 1 + nCorners) % nCorners,
                        edge: -1,
                    };
                    mesh.halfEdges.push(he);
                    dst.halfEdges.push(firstHE + k);
                    mesh.vertexHalves[he.origin].push(firstHE + k);
                }
            }
            if (!dst.halfEdges.length) {
                throw new Error('face ' + fi + ' has no loop with 3 or more corners');
            }
            mesh.faces.push(dst);
        }

        var dirKey = new Map();
        for (var i = 0; i < mesh.halfEdges.length; i++) {
            var half = mesh.halfEdges[i];
            var key = directedKey(half.origin, half.dest);
            if (dirKey.has(key)) {
                var other = mesh.halfEdges[dirKey.get(key)];
                throw new Error(
                    'duplicate directed edge ' + half.origin + '→' + half.dest +
                    ' (faces ' + half.face + ', ' + other.face + ')'
                );
            }
            dirKey.set(key, i);
        }

        for (var a = 0; a < mesh.halfEdges.length; a++) {
            var heA = mesh.halfEdges[a];
            if (heA.twin >= 0) continue;
            var twinKey = directedKey(heA.dest, heA.origin);
            if (!dirKey.has(twinKey)) continue;
            var b = dirKey.get(twinKey);
            if (a === b) continue;
            var edge = {
                halfA: a < b ? a : b,
                halfB: a < b ? b : a,
            };
            var edgeIdx = mesh.edges.length;
            mesh.edges.push(edge);
            mesh.halfEdges[a].twin = b;
            mesh.halfEdges[b].twin = a;
            mesh.halfEdges[a].edge = edgeIdx;
            mesh.halfEdges[b].edge = edgeIdx;
        }

        for (var h = 0; h < mesh.halfEdges.length; h++) {
            if (mesh.halfEdges[h].edge >= 0) continue;
            var boundary = mesh.edges.length;
            mesh.edges.push({ halfA: h, halfB: -1 });
            mesh.halfEdges[h].edge = boundary;
        }

        return mesh;
    }

    function isSlitEdge(mesh, edgeIdx) {
        var edge = mesh.edges[edgeIdx];
        if (edge.halfB < 0) return false;
        return mesh.halfEdges[edge.halfA].face === mesh.halfEdges[edge.halfB].face;
    }

    function fanHasEdge(mesh, fan, edgeIdx) {
        for (var i = 0; i < fan.length; i++) {
            if (mesh.halfEdges[fan[i]].edge === edgeIdx) return true;
        }
        return false;
    }

    function addIncomingBoundaryHalves(mesh, vertexIdx, fan) {
        if (!fan.length) return;
        function maybeAdd(neighborHe, atFront) {
            var incoming = mesh.halfEdges[neighborHe].prev;
            if (incoming < 0) return;
            var he = mesh.halfEdges[incoming];
            if (he.dest !== vertexIdx) return;
            if (fanHasEdge(mesh, fan, he.edge)) return;
            if (atFront) fan.unshift(incoming);
            else fan.push(incoming);
        }
        maybeAdd(fan[0], true);
        maybeAdd(fan[fan.length - 1], false);
    }

    function walkVertexFan(mesh, vertexIdx) {
        var outgoing = mesh.vertexHalves[vertexIdx];
        if (!outgoing.length) return [];
        var n = outgoing.length;
        var start = outgoing[0];
        var seen = new Uint8Array(mesh.halfEdges.length);
        seen[start] = 1;

        function stepPrevTwin(h) {
            var prev = mesh.halfEdges[h].prev;
            if (prev < 0) return -1;
            return mesh.halfEdges[prev].twin;
        }
        function stepTwinNext(h) {
            var twin = mesh.halfEdges[h].twin;
            return twin < 0 ? -1 : mesh.halfEdges[twin].next;
        }

        var after = [];
        var cur = start;
        for (var i = 0; i < n; i++) {
            var nxt = stepPrevTwin(cur);
            if (nxt < 0) break;
            if (mesh.halfEdges[nxt].origin !== vertexIdx) return [];
            if (nxt === start) {
                if (after.length + 1 !== n) return [];
                return [start].concat(after);
            }
            if (seen[nxt]) return [];
            seen[nxt] = 1;
            after.push(nxt);
            cur = nxt;
        }

        var before = [];
        cur = start;
        for (var j = 0; j < n; j++) {
            var prevStep = stepTwinNext(cur);
            if (prevStep < 0) break;
            if (mesh.halfEdges[prevStep].origin !== vertexIdx) return [];
            if (prevStep === start || seen[prevStep]) return [];
            seen[prevStep] = 1;
            before.push(prevStep);
            cur = prevStep;
        }

        var fan = [];
        for (var k = before.length - 1; k >= 0; k--) fan.push(before[k]);
        fan.push(start);
        for (var a = 0; a < after.length; a++) fan.push(after[a]);
        if (fan.length !== n) return [];
        addIncomingBoundaryHalves(mesh, vertexIdx, fan);
        return fan;
    }

    function dropSlitHalves(mesh, fan) {
        var kept = [];
        for (var i = 0; i < fan.length; i++) {
            if (isSlitEdge(mesh, mesh.halfEdges[fan[i]].edge)) continue;
            kept.push(fan[i]);
        }
        return kept;
    }

    function sub(a, b) {
        return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
    }

    function normalize(v) {
        var len = Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len === 0) return { x: 0, y: 0, z: 0 };
        return { x: v.x / len, y: v.y / len, z: v.z / len };
    }

    function cmpQuant(a, b) {
        if (a[0] !== b[0]) return a[0] < b[0] ? -1 : 1;
        if (a[1] !== b[1]) return a[1] < b[1] ? -1 : 1;
        if (a[2] !== b[2]) return a[2] < b[2] ? -1 : 1;
        return 0;
    }

    // Reversing an edge swaps its faces and negates its direction. Those two
    // readings are the same type. Keep the lexicographically greater quantized
    // direction. When the directions tie, keep the smaller (left, right) pair.
    function canonicalEdgeOrientation(left, right, dir) {
        var neg = { x: -dir.x, y: -dir.y, z: -dir.z };
        var qdir = [quantize(dir.x, DIR_EPS), quantize(dir.y, DIR_EPS), quantize(dir.z, DIR_EPS)];
        var qneg = [quantize(neg.x, DIR_EPS), quantize(neg.y, DIR_EPS), quantize(neg.z, DIR_EPS)];
        var cmp = cmpQuant(qdir, qneg);
        var reversed = cmp < 0 || (cmp === 0 && right < left);
        if (!reversed) {
            return { reversed: false, left: left, right: right, qdir: qdir.join(',') };
        }
        return { reversed: true, left: right, right: left, qdir: qneg.join(',') };
    }

    function extract(obj, mesh) {
        var faceTypeOfFace = new Array(mesh.faces.length);
        var faceTypeMap = new Map();
        var faceTypeCount = 0;
        for (var fi = 0; fi < mesh.faces.length; fi++) {
            var face = mesh.faces[fi];
            var key = face.material + '\0' + face.volAbove + '\0' + face.volBelow + '\0' + qkey(face.normal, NORMAL_EPS);
            var idx = faceTypeMap.get(key);
            if (idx === undefined) {
                idx = faceTypeCount++;
                faceTypeMap.set(key, idx);
            }
            faceTypeOfFace[fi] = idx;
        }

        var edgeTypeOfEdge = new Array(mesh.edges.length).fill(-1);
        var edgeDirs = new Array(mesh.edges.length);
        // 1 when this edge's halfA direction is opposite the type's canonical dir.
        var edgeReversed = new Array(mesh.edges.length).fill(0);
        var edgeTypeMap = new Map();
        var edgeTypeCount = 0;
        for (var ei = 0; ei < mesh.edges.length; ei++) {
            if (isSlitEdge(mesh, ei)) continue;
            var edge = mesh.edges[ei];
            var hA = mesh.halfEdges[edge.halfA];
            var dir = normalize(sub(obj.vertices[hA.dest], obj.vertices[hA.origin]));
            edgeDirs[ei] = dir;
            var leftFaceType = faceTypeOfFace[hA.face];
            var rightFaceType = edge.halfB >= 0 ? faceTypeOfFace[mesh.halfEdges[edge.halfB].face] : -1;
            var oriented = canonicalEdgeOrientation(leftFaceType, rightFaceType, dir);
            edgeReversed[ei] = oriented.reversed ? 1 : 0;
            var edgeKey = oriented.left + '\0' + oriented.right + '\0' + oriented.qdir;
            var edgeIdx = edgeTypeMap.get(edgeKey);
            if (edgeIdx === undefined) {
                edgeIdx = edgeTypeCount++;
                edgeTypeMap.set(edgeKey, edgeIdx);
            }
            edgeTypeOfEdge[ei] = edgeIdx;
        }

        var vertexTypeOfVertex = new Array(obj.vertices.length).fill(-1);
        var vertexSignatures = new Array(obj.vertices.length);
        var vertexTypeMap = new Map();
        var vertexTypeCount = 0;

        for (var v = 0; v < obj.vertices.length; v++) {
            if (!mesh.vertexHalves[v].length) continue;
            var fan = walkVertexFan(mesh, v);
            if (!fan.length) {
                throw new Error('vertex ' + v + ': cannot enumerate fan (non-manifold)');
            }
            fan = dropSlitHalves(mesh, fan);

            var rawSlots = [];
            for (var s = 0; s < fan.length; s++) {
                var heIndex = fan[s];
                var edgeIndex = mesh.halfEdges[heIndex].edge;
                if (isSlitEdge(mesh, edgeIndex)) continue;
                var etIdx = edgeTypeOfEdge[edgeIndex];
                if (etIdx < 0) continue;
                var atHalfAOrigin = mesh.halfEdges[mesh.edges[edgeIndex].halfA].origin === v ? 1 : 0;
                var isAtStart = edgeReversed[edgeIndex] ? 1 - atHalfAOrigin : atHalfAOrigin;
                rawSlots.push({ edgeType: etIdx, isAtStart: isAtStart });
            }
            if (!rawSlots.length) continue;

            var count = rawSlots.length;
            var best = 0;
            for (var rot = 1; rot < count; rot++) {
                for (var k = 0; k < count; k++) {
                    var slotA = rawSlots[(best + k) % count];
                    var slotB = rawSlots[(rot + k) % count];
                    if (slotA.edgeType !== slotB.edgeType) {
                        if (slotB.edgeType < slotA.edgeType) best = rot;
                        break;
                    }
                    if (slotA.isAtStart !== slotB.isAtStart) {
                        if (slotB.isAtStart < slotA.isAtStart) best = rot;
                        break;
                    }
                }
            }

            var tokens = [];
            var signature = [];
            for (var t = 0; t < count; t++) {
                var slot = rawSlots[(best + t) % count];
                tokens.push(slot.edgeType + ':' + slot.isAtStart);
                signature.push({ edge: slot.edgeType, isAtStart: slot.isAtStart === 1 });
            }
            var canon = tokens.join(',');
            var vtIdx = vertexTypeMap.get(canon);
            if (vtIdx === undefined) {
                vtIdx = vertexTypeCount++;
                vertexTypeMap.set(canon, vtIdx);
            }
            vertexTypeOfVertex[v] = vtIdx;
            vertexSignatures[v] = signature;
        }

        return {
            faceTypeOfFace: faceTypeOfFace,
            edgeTypeOfEdge: edgeTypeOfEdge,
            edgeDirs: edgeDirs,
            vertexTypeOfVertex: vertexTypeOfVertex,
            vertexSignatures: vertexSignatures,
            faceTypeCount: faceTypeCount,
            edgeTypeCount: edgeTypeCount,
            vertexTypeCount: vertexTypeCount,
        };
    }

    function loopsOfFace(mesh, faceIndex) {
        var face = mesh.faces[faceIndex];
        var seen = new Uint8Array(mesh.halfEdges.length);
        var loops = [];
        for (var i = 0; i < face.halfEdges.length; i++) {
            var start = face.halfEdges[i];
            if (seen[start]) continue;
            var loop = [];
            var h = start;
            do {
                seen[h] = 1;
                loop.push(mesh.halfEdges[h].origin);
                h = mesh.halfEdges[h].next;
            } while (h !== start && !seen[h]);
            if (loop.length >= 3) loops.push(loop);
        }
        return loops;
    }

    // u × v = normal. Dropping Y and keeping (x, z) mirrors the polygon,
    // because X × Z points along -Y. Earcut then reads reflex corners and
    // holes backwards on faces whose normal is along Y.
    function polygonBasis(normal) {
        var len = Math.sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        var n = len > 0 ? { x: normal.x / len, y: normal.y / len, z: normal.z / len } : { x: 0, y: 0, z: 1 };
        var ax = Math.abs(n.x);
        var ay = Math.abs(n.y);
        var az = Math.abs(n.z);
        var reference = ax <= ay && ax <= az ? { x: 1, y: 0, z: 0 }
            : ay <= az ? { x: 0, y: 1, z: 0 }
            : { x: 0, y: 0, z: 1 };
        var along = reference.x * n.x + reference.y * n.y + reference.z * n.z;
        var u = {
            x: reference.x - n.x * along,
            y: reference.y - n.y * along,
            z: reference.z - n.z * along,
        };
        var ulen = Math.sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
        if (ulen < 1e-12) {
            u = { x: 0, y: 1, z: 0 };
            ulen = 1;
        }
        u = { x: u.x / ulen, y: u.y / ulen, z: u.z / ulen };
        var v = {
            x: n.y * u.z - n.z * u.y,
            y: n.z * u.x - n.x * u.z,
            z: n.x * u.y - n.y * u.x,
        };
        return { u: u, v: v };
    }

    function ringArea(ring) {
        var sum = 0;
        for (var i = 0; i < ring.length; i++) {
            var a = ring[i];
            var b = ring[(i + 1) % ring.length];
            sum += a.x * b.y - b.x * a.y;
        }
        return sum;
    }

    function triangulateFace(vertices, loops, normal) {
        if (!earcut || !loops || !loops.length) return [];
        var basis = polygonBasis(normal);
        var rings = [];
        for (var li = 0; li < loops.length; li++) {
            var loop = loops[li];
            if (loop.length < 3) continue;
            var ring = [];
            for (var i = 0; i < loop.length; i++) {
                var p = vertices[loop[i]];
                ring.push({
                    x: p.x * basis.u.x + p.y * basis.u.y + p.z * basis.u.z,
                    y: p.x * basis.v.x + p.y * basis.v.y + p.z * basis.v.z,
                    index: loop[i],
                });
            }
            rings.push(ring);
        }
        if (!rings.length) return [];

        var areas = rings.map(ringArea);
        var outer = 0;
        for (var r = 1; r < rings.length; r++) {
            if (Math.abs(areas[r]) > Math.abs(areas[outer])) outer = r;
        }

        var data = [];
        var map = [];
        var holes = [];
        function append(ring) {
            for (var i = 0; i < ring.length; i++) {
                data.push(ring[i].x, ring[i].y);
                map.push(ring[i].index);
            }
        }
        append(rings[outer]);
        for (var h = 0; h < rings.length; h++) {
            if (h === outer || Math.abs(areas[h]) < 1e-8) continue;
            holes.push(map.length);
            append(rings[h]);
        }

        var indices = earcut(data, holes.length ? holes : null, 2);
        var triangles = [];
        for (var t = 0; t + 2 < indices.length; t += 3) {
            var i0 = map[indices[t]];
            var i1 = map[indices[t + 1]];
            var i2 = map[indices[t + 2]];
            if (i0 === i1 || i1 === i2 || i2 === i0) continue;
            triangles.push([i0, i1, i2]);
        }
        return triangles;
    }

    function loadMesh(objText) {
        try {
            var obj = parseObj(objText);
            var mesh = buildHalfEdgeMesh(obj);
            var extracted = extract(obj, mesh);
            var faces = [];
            for (var fi = 0; fi < mesh.faces.length; fi++) {
                var loops = loopsOfFace(mesh, fi);
                faces.push({
                    loops: loops,
                    triangles: triangulateFace(obj.vertices, loops, mesh.faces[fi].normal),
                    normal: mesh.faces[fi].normal,
                    material: mesh.faces[fi].material,
                    typeIndex: extracted.faceTypeOfFace[fi],
                });
            }
            var edges = [];
            for (var ei = 0; ei < mesh.edges.length; ei++) {
                if (extracted.edgeTypeOfEdge[ei] < 0) continue;
                var hA = mesh.halfEdges[mesh.edges[ei].halfA];
                edges.push({
                    a: hA.origin,
                    b: hA.dest,
                    dir: extracted.edgeDirs[ei],
                    typeIndex: extracted.edgeTypeOfEdge[ei],
                });
            }
            var vertices = obj.vertices.map(function (v, index) {
                return {
                    x: v.x,
                    y: v.y,
                    z: v.z,
                    typeIndex: extracted.vertexTypeOfVertex[index],
                    signature: extracted.vertexSignatures[index] || null,
                };
            });
            return {
                view: {
                    vertices: vertices,
                    faces: faces,
                    edges: edges,
                    faceTypeCount: extracted.faceTypeCount,
                    edgeTypeCount: extracted.edgeTypeCount,
                    vertexTypeCount: extracted.vertexTypeCount,
                },
            };
        } catch (err) {
            return { error: err.message || String(err) };
        }
    }

    function dot(a, b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    function countUnspliced(list) {
        var count = 0;
        for (var i = 0; i < list.length; i++) {
            if (!list[i] || !list[i].spliced) count++;
        }
        return count;
    }

    function countError(view, types) {
        if (!types || !Array.isArray(types.faceTypes) || !Array.isArray(types.edgeTypes) || !Array.isArray(types.vertexTypes)) {
            return 'JSON has no face, edge, and vertex types';
        }
        if (view.faceTypeCount !== types.faceTypes.length) {
            return 'face type count is ' + view.faceTypeCount + ' in the OBJ and ' + types.faceTypes.length + ' in the JSON';
        }
        // The rule generator appends spliced edge and vertex types after the
        // ones that come from the mesh. They have no element in the OBJ, so they
        // are not part of the count.
        var edgeCount = countUnspliced(types.edgeTypes);
        if (view.edgeTypeCount !== edgeCount) {
            return 'edge type count is ' + view.edgeTypeCount + ' in the OBJ and ' + edgeCount + ' in the JSON';
        }
        var vertexCount = countUnspliced(types.vertexTypes);
        if (view.vertexTypeCount !== vertexCount) {
            return 'vertex type count is ' + view.vertexTypeCount + ' in the OBJ and ' + vertexCount + ' in the JSON';
        }
        return '';
    }

    function alignmentError(view, types) {
        var counts = countError(view, types);
        if (counts) return counts;
        for (var fi = 0; fi < view.faces.length; fi++) {
            var face = view.faces[fi];
            var faceType = types.faceTypes[face.typeIndex];
            if (!faceType) return 'face ' + fi + ' has no face type';
            if ((faceType.material || '') !== (face.material || '')) {
                return 'face ' + fi + ' material does not match face type ' + face.typeIndex;
            }
            if (!faceType.normal || dot(face.normal, faceType.normal) < 1 - 1e-4) {
                return 'face ' + fi + ' normal does not match face type ' + face.typeIndex;
            }
        }
        for (var ei = 0; ei < view.edges.length; ei++) {
            var edge = view.edges[ei];
            var edgeType = types.edgeTypes[edge.typeIndex];
            if (!edgeType || !edgeType.dir || Math.abs(dot(edge.dir, edgeType.dir)) < 1 - 1e-4) {
                return 'edge ' + ei + ' direction does not match edge type ' + edge.typeIndex;
            }
        }
        for (var vi = 0; vi < view.vertices.length; vi++) {
            var vertex = view.vertices[vi];
            if (vertex.typeIndex < 0) continue;
            var vertexType = types.vertexTypes[vertex.typeIndex];
            var halfs = vertexType && vertexType.halfEdgeTypes;
            if (!halfs || !vertex.signature || halfs.length !== vertex.signature.length) {
                return 'vertex ' + vi + ' does not match vertex type ' + vertex.typeIndex;
            }
            for (var h = 0; h < halfs.length; h++) {
                if (halfs[h].edge !== vertex.signature[h].edge ||
                    !!halfs[h].isAtStart !== !!vertex.signature[h].isAtStart) {
                    return 'vertex ' + vi + ' half-edge ' + h + ' does not match vertex type ' + vertex.typeIndex;
                }
            }
        }
        return '';
    }

    return {
        loadMesh: loadMesh,
        countError: countError,
        alignmentError: alignmentError,
    };
});
