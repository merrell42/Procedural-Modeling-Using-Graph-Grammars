// Mesh topology for the primitives editor.
//
// Type indices follow cpp_version/mesh_extraction (half_edge_mesh.cpp and
// type_extractor.cpp), with useGroupAsVolume = false. A primitives JSON does
// not store which face, edge, or vertex uses which type, so a click has to
// recover that assignment from the embedded OBJ. OBJ polylines (`l`) are
// included: each segment takes the face type on its left and its right, and
// a segment drawn across a face has that face type on both sides. A corner
// that only the polyline uses gets a vertex type from the segments that meet
// there.

(function (root, factory) {
    if (typeof module === 'object' && module.exports) {
        module.exports = factory(require('./earcut.js'));
    } else {
        root.PrimitivesMesh = factory(root.earcut);
    }
})(typeof self !== 'undefined' ? self : this, function (earcut) {
    var NORMAL_EPS = 1e-4;
    var DIR_EPS = 1e-4;
    var POSITION_EPS = 1e-4;

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
        var polylines = [];
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
            } else if (key === 'l' && parts.length >= 3) {
                var chain = [];
                for (var j = 1; j < parts.length; j++) {
                    var lineToken = parts[j].split('/')[0];
                    if (!lineToken) continue;
                    var lineIndex = parseInt(lineToken, 10);
                    if (lineIndex > 0) lineIndex = lineIndex - 1;
                    else if (lineIndex < 0) lineIndex = vertices.length + lineIndex;
                    else continue;
                    chain.push(lineIndex);
                }
                if (chain.length >= 2) polylines.push(chain);
            }
        }
        if (!vertices.length || !faces.length) {
            throw new Error('OBJ has no vertices or faces');
        }
        return { vertices: vertices, faces: faces, polylines: polylines };
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
        if (right < 0) {
            return { reversed: false, left: left, right: right, qdir: qdir.join(',') };
        }
        var qneg = [quantize(neg.x, DIR_EPS), quantize(neg.y, DIR_EPS), quantize(neg.z, DIR_EPS)];
        var cmp = cmpQuant(qdir, qneg);
        var reversed = cmp < 0 || (cmp === 0 && right < left);
        if (!reversed) {
            return { reversed: false, left: left, right: right, qdir: qdir.join(',') };
        }
        return { reversed: true, left: right, right: left, qdir: qneg.join(',') };
    }

    function cross3(a, b) {
        return {
            x: a.y * b.z - a.z * b.y,
            y: a.z * b.x - a.x * b.z,
            z: a.x * b.y - a.y * b.x,
        };
    }

    function tangentU(n) {
        var ax = Math.abs(n.x);
        var ay = Math.abs(n.y);
        var az = Math.abs(n.z);
        var a = ax <= ay && ax <= az ? { x: 1, y: 0, z: 0 }
            : ay <= az ? { x: 0, y: 1, z: 0 }
            : { x: 0, y: 0, z: 1 };
        var u = cross3(n, a);
        var len = Math.hypot(u.x, u.y, u.z);
        if (len <= 1e-15) return { x: 1, y: 0, z: 0 };
        return { x: u.x / len, y: u.y / len, z: u.z / len };
    }

    function planeDist(geom, p) {
        return dot(sub(p, geom.origin), geom.normal);
    }

    function projectFace(geom, p) {
        var d = sub(p, geom.origin);
        return { x: dot(d, geom.axisU), y: dot(d, geom.axisV) };
    }

    function buildFaceGeom(mesh, obj, faceIdx, tol) {
        var geom = { valid: false, normal: mesh.faces[faceIdx].normal, loops: [], loopVerts: [] };
        if (Math.hypot(geom.normal.x, geom.normal.y, geom.normal.z) <= 1e-8) return geom;
        var loopVerts = loopsOfFace(mesh, faceIdx);
        if (!loopVerts.length) return geom;
        geom.origin = obj.vertices[loopVerts[0][0]];
        geom.axisU = tangentU(geom.normal);
        var axisV = cross3(geom.normal, geom.axisU);
        var vlen = Math.hypot(axisV.x, axisV.y, axisV.z);
        if (vlen <= 1e-15) return geom;
        geom.axisV = { x: axisV.x / vlen, y: axisV.y / vlen, z: axisV.z / vlen };
        geom.loopVerts = loopVerts;
        var maxDev = 0;
        for (var li = 0; li < loopVerts.length; li++) {
            var verts = loopVerts[li];
            var loop = [];
            for (var i = 0; i < verts.length; i++) {
                var v = verts[i];
                if (v < 0 || v >= obj.vertices.length) return { valid: false, loops: [], loopVerts: [] };
                var p = obj.vertices[v];
                maxDev = Math.max(maxDev, Math.abs(planeDist(geom, p)));
                loop.push(projectFace(geom, p));
            }
            geom.loops.push(loop);
        }
        // A bent face's own corners already leave the average plane.
        geom.planeTol = maxDev + tol;
        geom.valid = true;
        return geom;
    }

    function distPointSeg2(p, a, b) {
        var abx = b.x - a.x;
        var aby = b.y - a.y;
        var len2 = abx * abx + aby * aby;
        if (len2 <= 1e-30) return Math.hypot(p.x - a.x, p.y - a.y);
        var t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2;
        if (t < 0) t = 0;
        else if (t > 1) t = 1;
        return Math.hypot(p.x - (a.x + t * abx), p.y - (a.y + t * aby));
    }

    function onBoundary2D(geom, p, tol) {
        for (var li = 0; li < geom.loops.length; li++) {
            var loop = geom.loops[li];
            for (var i = 0; i < loop.length; i++) {
                if (distPointSeg2(p, loop[i], loop[(i + 1) % loop.length]) <= tol) return true;
            }
        }
        return false;
    }

    function evenOddInside(geom, p) {
        var crossings = 0;
        for (var li = 0; li < geom.loops.length; li++) {
            var loop = geom.loops[li];
            for (var i = 0; i < loop.length; i++) {
                var a = loop[i];
                var b = loop[(i + 1) % loop.length];
                if ((a.y > p.y) === (b.y > p.y)) continue;
                var t = (p.y - a.y) / (b.y - a.y);
                if (a.x + t * (b.x - a.x) > p.x) crossings++;
            }
        }
        return (crossings & 1) === 1;
    }

    function pointOnFace(geom, p, tol) {
        if (!geom.valid) return false;
        if (Math.abs(planeDist(geom, p)) > geom.planeTol) return false;
        var p2 = projectFace(geom, p);
        if (onBoundary2D(geom, p2, tol)) return true;
        return evenOddInside(geom, p2);
    }

    // 'left' / 'right': the point lies on a boundary half-edge parallel to dir.
    // The face interior is on the left of that half-edge. 'both': interior.
    function sideAt(geom, obj, p2, seg2, dir, tol) {
        var onLeft = false;
        var onRight = false;
        var slen = Math.hypot(seg2.x, seg2.y);
        for (var li = 0; li < geom.loopVerts.length; li++) {
            var verts = geom.loopVerts[li];
            var loop = geom.loops[li];
            for (var i = 0; i < verts.length; i++) {
                var c = loop[i];
                var d = loop[(i + 1) % loop.length];
                var elen = Math.hypot(d.x - c.x, d.y - c.y);
                if (elen <= tol || slen <= tol) continue;
                var cr = (d.x - c.x) * seg2.y - (d.y - c.y) * seg2.x;
                if (Math.abs(cr) / (elen * slen) > 1e-3) continue;
                if (distPointSeg2(p2, c, d) > tol) continue;
                var e0 = obj.vertices[verts[i]];
                var e1 = obj.vertices[verts[(i + 1) % verts.length]];
                var edge = sub(e1, e0);
                var el3 = Math.hypot(edge.x, edge.y, edge.z);
                if (el3 <= tol) continue;
                if (dot(edge, dir) / el3 >= 0) onLeft = true;
                else onRight = true;
            }
        }
        if (onLeft && onRight) return 'both';
        if (onLeft) return 'left';
        if (onRight) return 'right';
        if (evenOddInside(geom, p2)) return 'both';
        return 'none';
    }

    function addSegmentCuts(a, b, c, d, tol, ts) {
        var r = { x: b.x - a.x, y: b.y - a.y };
        var s = { x: d.x - c.x, y: d.y - c.y };
        var rxs = r.x * s.y - r.y * s.x;
        var rlen = Math.hypot(r.x, r.y);
        var slen = Math.hypot(s.x, s.y);
        if (rlen <= tol || slen <= tol) return;
        var qp = { x: c.x - a.x, y: c.y - a.y };
        if (Math.abs(rxs) <= tol * Math.max(rlen, slen)) {
            var qpxr = qp.x * r.y - qp.y * r.x;
            if (Math.abs(qpxr) > tol * rlen) return;
            var rr = r.x * r.x + r.y * r.y;
            var t0 = (qp.x * r.x + qp.y * r.y) / rr;
            var t1 = ((d.x - a.x) * r.x + (d.y - a.y) * r.y) / rr;
            var lo = Math.max(0, Math.min(t0, t1));
            var hi = Math.min(1, Math.max(t0, t1));
            if (hi > lo) {
                ts.push(lo);
                ts.push(hi);
            }
            return;
        }
        var t = (qp.x * s.y - qp.y * s.x) / rxs;
        var u = (qp.x * r.y - qp.y * r.x) / rxs;
        var tPad = tol / rlen;
        var uPad = tol / slen;
        if (t < -tPad || t > 1 + tPad || u < -uPad || u > 1 + uPad) return;
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        ts.push(t);
    }

    function sampleSides(obj, geoms, candidates, faceTypeOfFace, segA, segB, point, dir, tol) {
        var haveLeft = false;
        var haveRight = false;
        var left = -1;
        var right = -1;
        var hit = false;
        for (var ci = 0; ci < candidates.length; ci++) {
            var fi = candidates[ci];
            var geom = geoms[fi];
            if (!pointOnFace(geom, point, tol)) continue;
            var a2 = projectFace(geom, segA);
            var b2 = projectFace(geom, segB);
            var p2 = projectFace(geom, point);
            var side = sideAt(geom, obj, p2, { x: b2.x - a2.x, y: b2.y - a2.y }, dir, tol);
            if (side === 'none') continue;
            hit = true;
            var faceType = faceTypeOfFace[fi];
            function take(have, slot) {
                if (!have) return { have: true, slot: faceType, ok: true };
                return { have: true, slot: slot, ok: slot === faceType };
            }
            if (side === 'left' || side === 'both') {
                var takenL = take(haveLeft, left);
                if (!takenL.ok) return { kind: 'conflict' };
                haveLeft = takenL.have;
                left = takenL.slot;
            }
            if (side === 'right' || side === 'both') {
                var takenR = take(haveRight, right);
                if (!takenR.ok) return { kind: 'conflict' };
                haveRight = takenR.have;
                right = takenR.slot;
            }
        }
        if (!hit) return { kind: 'off' };
        return { kind: 'ready', left: haveLeft ? left : -1, right: haveRight ? right : -1 };
    }

    // Intern each OBJ polyline segment. A segment may cross coplanar faces of
    // one face type; the left/right pair has to stay the same on every piece.
    function classifyPolylines(obj, mesh, faceTypeOfFace, edgeTypeMap, edgeTypeCount) {
        var tol = POSITION_EPS;
        var geoms = [];
        for (var fi = 0; fi < mesh.faces.length; fi++) {
            geoms.push(buildFaceGeom(mesh, obj, fi, tol));
        }
        var edges = [];
        var corners = [];
        function fail(pi, va, vb, why) {
            throw new Error('polyline ' + pi + ' segment ' + (va + 1) + ' -> ' + (vb + 1) + ' ' + why);
        }
        for (var pi = 0; pi < obj.polylines.length; pi++) {
            var poly = obj.polylines[pi];
            for (var s = 0; s + 1 < poly.length; s++) {
                var va = poly[s];
                var vb = poly[s + 1];
                if (va < 0 || vb < 0 || va >= obj.vertices.length || vb >= obj.vertices.length) {
                    throw new Error('polyline ' + pi + ' references an out-of-range vertex');
                }
                if (va === vb) continue;
                var A = obj.vertices[va];
                var B = obj.vertices[vb];
                var delta = sub(B, A);
                var segLen = Math.hypot(delta.x, delta.y, delta.z);
                if (segLen <= 1e-15) continue;
                var dir = { x: delta.x / segLen, y: delta.y / segLen, z: delta.z / segLen };
                var onA = false;
                var onB = false;
                for (var g = 0; g < geoms.length; g++) {
                    if (!onA && pointOnFace(geoms[g], A, tol)) onA = true;
                    if (!onB && pointOnFace(geoms[g], B, tol)) onB = true;
                }
                if (!onA || !onB) fail(pi, va, vb, 'does not lie on the mesh');

                var candidates = [];
                var ts = [0, 1];
                for (var f = 0; f < geoms.length; f++) {
                    var geom = geoms[f];
                    if (!geom.valid) continue;
                    if (Math.abs(planeDist(geom, A)) > geom.planeTol) continue;
                    if (Math.abs(planeDist(geom, B)) > geom.planeTol) continue;
                    candidates.push(f);
                    var a2 = projectFace(geom, A);
                    var b2 = projectFace(geom, B);
                    for (var li = 0; li < geom.loops.length; li++) {
                        var loop = geom.loops[li];
                        for (var i = 0; i < loop.length; i++) {
                            addSegmentCuts(a2, b2, loop[i], loop[(i + 1) % loop.length], tol, ts);
                        }
                    }
                }
                if (!candidates.length) fail(pi, va, vb, 'does not lie on the mesh');

                var tTol = Math.max(tol / segLen, 1e-9);
                ts.sort(function (p, q) { return p - q; });
                var cuts = [];
                for (var ti = 0; ti < ts.length; ti++) {
                    var tcut = ts[ti];
                    if (tcut < 0) tcut = 0;
                    if (tcut > 1) tcut = 1;
                    if (!cuts.length || tcut > cuts[cuts.length - 1] + tTol) cuts.push(tcut);
                }
                if (!cuts.length || cuts[0] > 0) cuts.unshift(0);
                if (cuts[cuts.length - 1] < 1) cuts.push(1);

                function atT(t) {
                    return {
                        x: A.x + delta.x * t,
                        y: A.y + delta.y * t,
                        z: A.z + delta.z * t,
                    };
                }
                var haveRef = false;
                var refLeft = -1;
                var refRight = -1;
                var sawSpan = false;
                for (var span = 0; span + 1 < cuts.length; span++) {
                    var t0 = cuts[span];
                    var t1 = cuts[span + 1];
                    if ((t1 - t0) * segLen <= tol) continue;
                    sawSpan = true;
                    var sample = sampleSides(
                        obj, geoms, candidates, faceTypeOfFace,
                        A, B, atT(0.5 * (t0 + t1)), dir, tol
                    );
                    if (sample.kind === 'conflict') {
                        fail(pi, va, vb, 'has more than one face type on the same side');
                    }
                    if (sample.kind === 'off') fail(pi, va, vb, 'does not lie on the mesh');
                    if (!haveRef) {
                        refLeft = sample.left;
                        refRight = sample.right;
                        haveRef = true;
                    } else if (sample.left !== refLeft || sample.right !== refRight) {
                        fail(pi, va, vb, 'crosses more than one pair of face types');
                    }
                }
                if (!sawSpan) {
                    var midSample = sampleSides(
                        obj, geoms, candidates, faceTypeOfFace, A, B, atT(0.5), dir, tol
                    );
                    if (midSample.kind !== 'ready') fail(pi, va, vb, 'does not lie on the mesh');
                    refLeft = midSample.left;
                    refRight = midSample.right;
                    haveRef = true;
                }
                if (!haveRef) fail(pi, va, vb, 'does not lie on the mesh');

                // A boundary mesh edge keeps its one face on the left of dir.
                var flipped = refLeft < 0 && refRight >= 0;
                var useDir = dir;
                var left = refLeft;
                var right = refRight;
                if (flipped) {
                    left = right;
                    right = -1;
                    useDir = { x: -dir.x, y: -dir.y, z: -dir.z };
                }
                var oriented = canonicalEdgeOrientation(left, right, useDir);
                var edgeKey = oriented.left + '\0' + oriented.right + '\0' + oriented.qdir;
                var typeIndex = edgeTypeMap.get(edgeKey);
                if (typeIndex === undefined) {
                    typeIndex = edgeTypeCount++;
                    edgeTypeMap.set(edgeKey, typeIndex);
                }
                var storedDir = oriented.reversed
                    ? { x: -useDir.x, y: -useDir.y, z: -useDir.z }
                    : useDir;
                edges.push({ a: va, b: vb, dir: storedDir, typeIndex: typeIndex });

                var aIsAtStart = flipped ? oriented.reversed : !oriented.reversed;
                var normal = { x: 0, y: 0, z: 1 };
                var mid = atT(0.5);
                for (var gi = 0; gi < geoms.length; gi++) {
                    if (pointOnFace(geoms[gi], mid, tol)) {
                        normal = geoms[gi].normal;
                        break;
                    }
                }
                corners.push({
                    vertex: va,
                    edgeType: typeIndex,
                    isAtStart: aIsAtStart ? 1 : 0,
                    outward: dir,
                    normal: normal,
                });
                corners.push({
                    vertex: vb,
                    edgeType: typeIndex,
                    isAtStart: aIsAtStart ? 0 : 1,
                    outward: { x: -dir.x, y: -dir.y, z: -dir.z },
                    normal: normal,
                });
            }
        }
        return { edgeTypeCount: edgeTypeCount, edges: edges, corners: corners };
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

        var polylineEdges = [];
        var polyCorners = [];
        if (obj.polylines && obj.polylines.length) {
            var classified = classifyPolylines(obj, mesh, faceTypeOfFace, edgeTypeMap, edgeTypeCount);
            edgeTypeCount = classified.edgeTypeCount;
            polylineEdges = classified.edges;
            polyCorners = classified.corners;
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

        var cornersByVertex = new Map();
        var polyVerts = [];
        for (var c = 0; c < polyCorners.length; c++) {
            var corner = polyCorners[c];
            var list = cornersByVertex.get(corner.vertex);
            if (!list) {
                list = [];
                cornersByVertex.set(corner.vertex, list);
                polyVerts.push(corner.vertex);
            }
            list.push(corner);
        }
        polyVerts.sort(function (a, b) { return a - b; });
        for (var pv = 0; pv < polyVerts.length; pv++) {
            var pvIndex = polyVerts[pv];
            if (vertexTypeOfVertex[pvIndex] >= 0) continue;
            var incident = cornersByVertex.get(pvIndex);
            var normal = incident[0].normal;
            var axisU = tangentU(normal);
            var axisV = cross3(normal, axisU);
            var vlen = Math.hypot(axisV.x, axisV.y, axisV.z);
            if (vlen > 1e-15) {
                axisV = { x: axisV.x / vlen, y: axisV.y / vlen, z: axisV.z / vlen };
            }
            var polySlots = incident.map(function (item) {
                return {
                    edgeType: item.edgeType,
                    isAtStart: item.isAtStart,
                    angle: Math.atan2(dot(item.outward, axisV), dot(item.outward, axisU)),
                };
            });
            polySlots.sort(function (a, b) { return a.angle - b.angle; });

            var polyCount = polySlots.length;
            var polyBest = 0;
            for (var prot = 1; prot < polyCount; prot++) {
                for (var pk = 0; pk < polyCount; pk++) {
                    var polyA = polySlots[(polyBest + pk) % polyCount];
                    var polyB = polySlots[(prot + pk) % polyCount];
                    if (polyA.edgeType !== polyB.edgeType) {
                        if (polyB.edgeType < polyA.edgeType) polyBest = prot;
                        break;
                    }
                    if (polyA.isAtStart !== polyB.isAtStart) {
                        if (polyB.isAtStart < polyA.isAtStart) polyBest = prot;
                        break;
                    }
                }
            }
            var polyTokens = [];
            var polySignature = [];
            for (var pt = 0; pt < polyCount; pt++) {
                var polySlot = polySlots[(polyBest + pt) % polyCount];
                polyTokens.push(polySlot.edgeType + ':' + polySlot.isAtStart);
                polySignature.push({ edge: polySlot.edgeType, isAtStart: polySlot.isAtStart === 1 });
            }
            var polyCanon = polyTokens.join(',');
            var polyVt = vertexTypeMap.get(polyCanon);
            if (polyVt === undefined) {
                polyVt = vertexTypeCount++;
                vertexTypeMap.set(polyCanon, polyVt);
            }
            vertexTypeOfVertex[pvIndex] = polyVt;
            vertexSignatures[pvIndex] = polySignature;
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
            polylineEdges: polylineEdges,
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
            var polyEdges = extracted.polylineEdges || [];
            for (var pe = 0; pe < polyEdges.length; pe++) edges.push(polyEdges[pe]);
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
