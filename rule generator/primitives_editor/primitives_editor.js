(function () {
    var canvas = document.getElementById('view');
    var openBtn = document.getElementById('openBtn');
    var downloadBtn = document.getElementById('downloadBtn');
    var fileInput = document.getElementById('fileInput');
    var fileStatus = document.getElementById('fileStatus');
    var loadError = document.getElementById('loadError');
    var empty = document.getElementById('empty');
    var workspace = document.getElementById('workspace');
    var matchWarning = document.getElementById('matchWarning');
    var typeSelect = document.getElementById('typeSelect');
    var typePicker = typeSelect.parentNode;
    var summary = document.getElementById('summary');
    var fields = document.getElementById('fields');
    var backBtn = document.getElementById('backBtn');
    var forwardBtn = document.getElementById('forwardBtn');

    var gl = canvas.getContext('webgl', { antialias: true });
    var litProgram = null;
    var unlitProgram = null;
    var buffers = {};

    var state = {
        doc: null,
        fileName: '',
        view: null,
        selection: null,
        hover: null,
    };

    var UP = { x: 0, y: 0, z: 1 };

    var camera = {
        target: { x: 0, y: 0, z: 0 },
        theta: 0.7,
        phi: 1.05,
        distance: 4,
        radius: 1,
        fovy: Math.PI / 4,
    };

    var drag = null;

    window.PrimitivesDecorations.init({
        doc: function () { return state.doc; },
        rerender: function () { renderFields(); },
        open: openDecoration,
        back: function () { stepSelection(-1); },
        renamed: renameDecorationInHistory,
    });

    function sub(a, b) {
        return { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
    }

    function add(a, b) {
        return { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z };
    }

    function scale(a, s) {
        return { x: a.x * s, y: a.y * s, z: a.z * s };
    }

    function dot(a, b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    function cross(a, b) {
        return {
            x: a.y * b.z - a.z * b.y,
            y: a.z * b.x - a.x * b.z,
            z: a.x * b.y - a.y * b.x,
        };
    }

    function normalize(v) {
        var len = Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len === 0) return { x: 0, y: 0, z: 0 };
        return scale(v, 1 / len);
    }

    function lookAt(eye, target, up) {
        var z = normalize(sub(eye, target));
        var x = normalize(cross(up, z));
        var y = cross(z, x);
        return [
            x.x, y.x, z.x, 0,
            x.y, y.y, z.y, 0,
            x.z, y.z, z.z, 0,
            -dot(x, eye), -dot(y, eye), -dot(z, eye), 1,
        ];
    }

    function perspective(fovy, aspect, near, far) {
        var f = 1 / Math.tan(fovy / 2);
        var nf = 1 / (near - far);
        return [
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (far + near) * nf, -1,
            0, 0, 2 * far * near * nf, 0,
        ];
    }

    function multiply(a, b) {
        var out = new Array(16);
        for (var c = 0; c < 4; c++) {
            for (var r = 0; r < 4; r++) {
                out[c * 4 + r] =
                    a[r] * b[c * 4] +
                    a[4 + r] * b[c * 4 + 1] +
                    a[8 + r] * b[c * 4 + 2] +
                    a[12 + r] * b[c * 4 + 3];
            }
        }
        return out;
    }

    function compileShader(type, source) {
        var shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            throw new Error(gl.getShaderInfoLog(shader) || 'shader compile failed');
        }
        return shader;
    }

    function linkProgram(vsSource, fsSource) {
        var program = gl.createProgram();
        gl.attachShader(program, compileShader(gl.VERTEX_SHADER, vsSource));
        gl.attachShader(program, compileShader(gl.FRAGMENT_SHADER, fsSource));
        gl.linkProgram(program);
        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            throw new Error(gl.getProgramInfoLog(program) || 'program link failed');
        }
        return program;
    }

    function initGl() {
        if (!gl) return;
        litProgram = linkProgram(
            'attribute vec3 aPos; attribute vec3 aNormal; attribute vec3 aColor; uniform mat4 uMvp; varying vec3 vColor; varying vec3 vNormal; void main() { gl_Position = uMvp * vec4(aPos, 1.0); vNormal = aNormal; vColor = aColor; }',
            'precision mediump float; varying vec3 vColor; varying vec3 vNormal; void main() { vec3 n = normalize(vNormal); vec3 light = normalize(vec3(0.35, 0.45, 0.8)); float shade = 0.38 + 0.62 * abs(dot(n, light)); gl_FragColor = vec4(vColor * shade, 1.0); }'
        );
        unlitProgram = linkProgram(
            'attribute vec3 aPos; attribute vec3 aColor; attribute float aPointSize; uniform mat4 uMvp; varying vec3 vColor; void main() { gl_Position = uMvp * vec4(aPos, 1.0); gl_PointSize = aPointSize; vColor = aColor; }',
            'precision mediump float; varying vec3 vColor; uniform float uRound; void main() { if (uRound > 0.5) { vec2 c = gl_PointCoord * 2.0 - 1.0; if (dot(c, c) > 1.0) discard; } gl_FragColor = vec4(vColor, 1.0); }'
        );
        gl.enable(gl.DEPTH_TEST);
        gl.depthFunc(gl.LEQUAL);
    }

    function upload(name, data) {
        if (!buffers[name]) buffers[name] = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffers[name]);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(data), gl.DYNAMIC_DRAW);
        return buffers[name];
    }

    function bindAttrib(program, name, buffer, size) {
        var loc = gl.getAttribLocation(program, name);
        if (loc < 0) return;
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.enableVertexAttribArray(loc);
        gl.vertexAttribPointer(loc, size, gl.FLOAT, false, 0, 0);
    }

    function disableAttribs(program) {
        ['aPos', 'aNormal', 'aColor', 'aPointSize'].forEach(function (name) {
            var loc = gl.getAttribLocation(program, name);
            if (loc >= 0) gl.disableVertexAttribArray(loc);
        });
    }

    function cssSize() {
        var rect = canvas.getBoundingClientRect();
        return {
            width: Math.max(rect.width, 1),
            height: Math.max(rect.height, 1),
        };
    }

    function resize() {
        if (!gl) return;
        var size = cssSize();
        var dpr = window.devicePixelRatio || 1;
        var w = Math.max(1, Math.floor(size.width * dpr));
        var h = Math.max(1, Math.floor(size.height * dpr));
        if (canvas.width !== w || canvas.height !== h) {
            canvas.width = w;
            canvas.height = h;
        }
        gl.viewport(0, 0, canvas.width, canvas.height);
    }

    function eyePosition() {
        var sinPhi = Math.sin(camera.phi);
        return {
            x: camera.target.x + camera.distance * sinPhi * Math.sin(camera.theta),
            y: camera.target.y + camera.distance * sinPhi * Math.cos(camera.theta),
            z: camera.target.z + camera.distance * Math.cos(camera.phi),
        };
    }

    function mvpMatrix() {
        var size = cssSize();
        var eye = eyePosition();
        var near = Math.max(camera.radius * 0.01, 0.01);
        var far = Math.max(camera.radius * 80, camera.distance * 8);
        var view = lookAt(eye, camera.target, UP);
        var proj = perspective(camera.fovy, size.width / size.height, near, far);
        return { mvp: multiply(proj, view), eye: eye, width: size.width, height: size.height };
    }

    function faceColor(typeIndex) {
        var types = state.doc.types.faceTypes || [];
        var color = types[typeIndex] && types[typeIndex].color;
        if (!color) return [0.75, 0.75, 0.75];
        return [
            Number.isFinite(color.x) ? color.x : 0.75,
            Number.isFinite(color.y) ? color.y : 0.75,
            Number.isFinite(color.z) ? color.z : 0.75,
        ];
    }

    function mix(a, b, t) {
        return [
            a[0] * (1 - t) + b[0] * t,
            a[1] * (1 - t) + b[1] * t,
            a[2] * (1 - t) + b[2] * t,
        ];
    }

    function hitIndex(hit, kind) {
        return hit && hit.kind === kind ? hit.elementIndex : -1;
    }

    function typeIndexOf(hit, kind) {
        return hit && hit.kind === kind && hit.typeIndex >= 0 ? hit.typeIndex : -1;
    }

    function displayFaceColor(faceIndex) {
        var face = state.view.faces[faceIndex];
        var color = faceColor(face.typeIndex);
        var selectedType = typeIndexOf(state.selection, 'face');
        var hoverType = typeIndexOf(state.hover, 'face');
        if (selectedType >= 0 && face.typeIndex === selectedType) {
            color = mix(color, [1, 0.78, 0.2], 0.4);
        }
        if (hoverType >= 0 && face.typeIndex === hoverType) {
            color = mix(color, [1, 0.95, 0.55], 0.55);
        }
        return color;
    }

    function pushRibbon(pos, col, a, b, pixels, color, frame) {
        var mid = {
            x: (a.x + b.x) / 2,
            y: (a.y + b.y) / 2,
            z: (a.z + b.z) / 2,
        };
        var eyeDir = sub(frame.eye, mid);
        var dist = Math.sqrt(dot(eyeDir, eyeDir)) || 1;
        var half = pixels * dist * Math.tan(camera.fovy / 2) / frame.height;
        var side = cross(sub(b, a), eyeDir);
        var slen = Math.sqrt(dot(side, side));
        if (slen < 1e-10) {
            side = cross(sub(b, a), UP);
            slen = Math.sqrt(dot(side, side));
        }
        if (slen < 1e-10) return;
        side = scale(side, half / slen);
        [sub(a, side), add(a, side), sub(b, side), add(a, side), add(b, side), sub(b, side)].forEach(function (v) {
            pos.push(v.x, v.y, v.z);
            col.push(color[0], color[1], color[2]);
        });
    }

    function draw() {
        if (!gl || !state.view || workspace.hidden) return;
        resize();
        var frame = mvpMatrix();
        gl.clearColor(0.875, 0.89, 0.91, 1);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        var facePos = [];
        var faceNrm = [];
        var faceCol = [];
        state.view.faces.forEach(function (face, faceIndex) {
            var color = displayFaceColor(faceIndex);
            var normal = face.normal;
            (face.triangles || []).forEach(function (tri) {
                tri.forEach(function (vi) {
                    var v = state.view.vertices[vi];
                    facePos.push(v.x, v.y, v.z);
                    faceNrm.push(normal.x, normal.y, normal.z);
                    faceCol.push(color[0], color[1], color[2]);
                });
            });
        });

        gl.useProgram(litProgram);
        gl.uniformMatrix4fv(gl.getUniformLocation(litProgram, 'uMvp'), false, new Float32Array(frame.mvp));
        gl.enable(gl.POLYGON_OFFSET_FILL);
        gl.polygonOffset(1, 1);
        if (facePos.length) {
            bindAttrib(litProgram, 'aPos', upload('facePos', facePos), 3);
            bindAttrib(litProgram, 'aNormal', upload('faceNrm', faceNrm), 3);
            bindAttrib(litProgram, 'aColor', upload('faceCol', faceCol), 3);
            gl.drawArrays(gl.TRIANGLES, 0, facePos.length / 3);
        }
        disableAttribs(litProgram);
        gl.disable(gl.POLYGON_OFFSET_FILL);

        var ribbonPos = [];
        var ribbonCol = [];
        function undirectedKey(i, j) {
            return i < j ? i + ':' + j : j + ':' + i;
        }
        function boundaryKeys(typeIndex) {
            var keys = {};
            if (typeIndex < 0) return keys;
            state.view.faces.forEach(function (face) {
                if (face.typeIndex !== typeIndex) return;
                face.loops.forEach(function (loop) {
                    for (var i = 0; i < loop.length; i++) {
                        keys[undirectedKey(loop[i], loop[(i + 1) % loop.length])] = true;
                    }
                });
            });
            return keys;
        }
        function pushBoundary(keys, skip, pixels, color) {
            Object.keys(keys).forEach(function (key) {
                if (skip[key]) return;
                var parts = key.split(':');
                pushRibbon(
                    ribbonPos, ribbonCol,
                    state.view.vertices[Number(parts[0])],
                    state.view.vertices[Number(parts[1])],
                    pixels, color, frame
                );
            });
        }
        var selectedFaceType = typeIndexOf(state.selection, 'face');
        var hoveredFaceType = typeIndexOf(state.hover, 'face');
        var selectedBoundary = boundaryKeys(selectedFaceType);
        var hoveredBoundary = boundaryKeys(hoveredFaceType);
        var edgeHighlightKeys = {};
        state.view.edges.forEach(function (edge) {
            var hoverType = typeIndexOf(state.hover, 'edge');
            var hovered = hoverType >= 0 && edge.typeIndex === hoverType;
            var selected = typeIndexOf(state.selection, 'edge') === edge.typeIndex && edge.typeIndex >= 0;
            var key = undirectedKey(edge.a, edge.b);
            if (selected || hovered) edgeHighlightKeys[key] = true;
            // A face outline is drawn on this segment. Skip the dark edge so the two ribbons do not z-fight.
            if ((selectedBoundary[key] || hoveredBoundary[key]) && !selected && !hovered) return;
            var color = [0.1, 0.12, 0.16];
            var thickness = 5;
            if (selected) {
                color = [0.95, 0.45, 0.05];
                thickness = 7;
            }
            if (hovered) {
                color = [1, 0.86, 0.25];
                thickness = 10;
            }
            pushRibbon(ribbonPos, ribbonCol, state.view.vertices[edge.a], state.view.vertices[edge.b], thickness, color, frame);
        });
        if (selectedFaceType >= 0 && selectedFaceType !== hoveredFaceType) {
            var skipSelected = {};
            Object.keys(edgeHighlightKeys).forEach(function (key) { skipSelected[key] = true; });
            Object.keys(hoveredBoundary).forEach(function (key) { skipSelected[key] = true; });
            pushBoundary(selectedBoundary, skipSelected, 6, [1, 0.92, 0.2]);
        }
        if (hoveredFaceType >= 0) {
            pushBoundary(hoveredBoundary, edgeHighlightKeys, 7, [1, 0.97, 0.55]);
        }

        gl.useProgram(unlitProgram);
        gl.uniformMatrix4fv(gl.getUniformLocation(unlitProgram, 'uMvp'), false, new Float32Array(frame.mvp));
        gl.uniform1f(gl.getUniformLocation(unlitProgram, 'uRound'), 0);
        gl.enable(gl.POLYGON_OFFSET_FILL);
        gl.polygonOffset(-1, -1);
        if (ribbonPos.length) {
            bindAttrib(unlitProgram, 'aPos', upload('ribbonPos', ribbonPos), 3);
            bindAttrib(unlitProgram, 'aColor', upload('ribbonCol', ribbonCol), 3);
            gl.drawArrays(gl.TRIANGLES, 0, ribbonPos.length / 3);
        }
        disableAttribs(unlitProgram);
        gl.disable(gl.POLYGON_OFFSET_FILL);

        gl.uniform1f(gl.getUniformLocation(unlitProgram, 'uRound'), 1);
        var pointPos = [];
        var pointCol = [];
        var pointSize = [];
        var dpr = window.devicePixelRatio || 1;
        state.view.vertices.forEach(function (vertex) {
            if (vertex.typeIndex < 0) return;
            var hoverType = typeIndexOf(state.hover, 'vertex');
            var hovered = hoverType >= 0 && vertex.typeIndex === hoverType;
            var selected = typeIndexOf(state.selection, 'vertex') === vertex.typeIndex;
            var css = 16;
            var color = [0.12, 0.14, 0.18];
            if (selected) {
                css = 24;
                color = [0.95, 0.45, 0.05];
            }
            if (hovered) {
                css = 32;
                color = [1, 0.9, 0.3];
            }
            pointPos.push(vertex.x, vertex.y, vertex.z);
            pointCol.push(color[0], color[1], color[2]);
            pointSize.push(css * dpr);
        });
        if (pointPos.length) {
            bindAttrib(unlitProgram, 'aPos', upload('ptPos', pointPos), 3);
            bindAttrib(unlitProgram, 'aColor', upload('ptCol', pointCol), 3);
            bindAttrib(unlitProgram, 'aPointSize', upload('ptSize', pointSize), 1);
            gl.drawArrays(gl.POINTS, 0, pointPos.length / 3);
        }
        disableAttribs(unlitProgram);
    }

    function project(vertex, frame) {
        var m = frame.mvp;
        var x = m[0] * vertex.x + m[4] * vertex.y + m[8] * vertex.z + m[12];
        var y = m[1] * vertex.x + m[5] * vertex.y + m[9] * vertex.z + m[13];
        var z = m[2] * vertex.x + m[6] * vertex.y + m[10] * vertex.z + m[14];
        var w = m[3] * vertex.x + m[7] * vertex.y + m[11] * vertex.z + m[15];
        if (w <= 1e-5) return null;
        return {
            x: (x / w * 0.5 + 0.5) * frame.width,
            y: (1 - (y / w * 0.5 + 0.5)) * frame.height,
            depth: z / w,
        };
    }

    function screenRay(px, py, frame) {
        var aspect = frame.width / frame.height;
        var tan = Math.tan(camera.fovy / 2);
        var ndcX = (px / frame.width) * 2 - 1;
        var ndcY = 1 - (py / frame.height) * 2;
        var backward = normalize(sub(frame.eye, camera.target));
        var xAxis = normalize(cross(UP, backward));
        var yAxis = cross(backward, xAxis);
        var dir = normalize(add(
            add(scale(xAxis, ndcX * aspect * tan), scale(yAxis, ndcY * tan)),
            scale(backward, -1)
        ));
        return { origin: frame.eye, dir: dir };
    }

    function rayTriangle(origin, dir, a, b, c) {
        var ab = sub(b, a);
        var ac = sub(c, a);
        var pvec = cross(dir, ac);
        var det = dot(ab, pvec);
        if (Math.abs(det) < 1e-8) return null;
        var invDet = 1 / det;
        var tvec = sub(origin, a);
        var u = dot(tvec, pvec) * invDet;
        if (u < -1e-5 || u > 1 + 1e-5) return null;
        var qvec = cross(tvec, ab);
        var v = dot(dir, qvec) * invDet;
        if (v < -1e-5 || u + v > 1 + 1e-5) return null;
        var t = dot(ac, qvec) * invDet;
        return t > 1e-4 ? t : null;
    }

    function pickFace(px, py, frame) {
        var ray = screenRay(px, py, frame);
        var best = null;
        state.view.faces.forEach(function (face, faceIndex) {
            var tris = face.triangles || [];
            for (var i = 0; i < tris.length; i++) {
                var tri = tris[i];
                var t = rayTriangle(
                    ray.origin,
                    ray.dir,
                    state.view.vertices[tri[0]],
                    state.view.vertices[tri[1]],
                    state.view.vertices[tri[2]]
                );
                if (t === null) continue;
                if (!best || t < best.t) best = { kind: 'face', t: t, elementIndex: faceIndex, typeIndex: face.typeIndex };
            }
        });
        return best;
    }

    function distToSegment(px, py, a, b) {
        var abx = b.x - a.x;
        var aby = b.y - a.y;
        var len2 = abx * abx + aby * aby;
        var t = len2 > 0 ? ((px - a.x) * abx + (py - a.y) * aby) / len2 : 0;
        t = Math.max(0, Math.min(1, t));
        var dx = a.x + abx * t - px;
        var dy = a.y + aby * t - py;
        return Math.sqrt(dx * dx + dy * dy);
    }

    function pickEdge(px, py, frame) {
        var best = null;
        state.view.edges.forEach(function (edge, edgeIndex) {
            var a = project(state.view.vertices[edge.a], frame);
            var b = project(state.view.vertices[edge.b], frame);
            if (!a || !b) return;
            if (a.depth < -1 && b.depth < -1) return;
            var dist = distToSegment(px, py, a, b);
            if (dist > 14) return;
            if (!best || dist < best.dist) {
                best = { kind: 'edge', dist: dist, elementIndex: edgeIndex, typeIndex: edge.typeIndex };
            }
        });
        return best;
    }

    function pickVertex(px, py, frame) {
        var best = null;
        state.view.vertices.forEach(function (vertex, index) {
            if (vertex.typeIndex < 0) return;
            var p = project(vertex, frame);
            if (!p || p.depth < -1 || p.depth > 1) return;
            var dist = Math.hypot(p.x - px, p.y - py);
            if (dist > 18) return;
            if (!best || dist < best.dist) {
                best = { kind: 'vertex', dist: dist, elementIndex: index, typeIndex: vertex.typeIndex };
            }
        });
        return best;
    }

    function pick(px, py) {
        var frame = mvpMatrix();
        return pickVertex(px, py, frame) || pickEdge(px, py, frame) || pickFace(px, py, frame);
    }

    function sameHit(a, b) {
        if (!a || !b) return !a && !b;
        return a.kind === b.kind && a.elementIndex === b.elementIndex;
    }

    function kindNoun(kind, count) {
        if (kind === 'face') return count === 1 ? 'face' : 'faces';
        if (kind === 'edge') return count === 1 ? 'edge' : 'edges';
        return count === 1 ? 'vertex' : 'vertices';
    }

    function kindLabel(kind) {
        if (kind === 'face') return 'Face type';
        if (kind === 'edge') return 'Edge type';
        return 'Vertex type';
    }

    function elements(kind) {
        if (kind === 'face') return state.view.faces;
        if (kind === 'edge') return state.view.edges;
        return state.view.vertices;
    }

    function usageCount(kind, typeIndex) {
        var count = 0;
        elements(kind).forEach(function (item) {
            if (item.typeIndex === typeIndex) count++;
        });
        return count;
    }

    function firstIndex(kind, typeIndex) {
        var items = elements(kind);
        for (var i = 0; i < items.length; i++) {
            if (items[i].typeIndex === typeIndex) return i;
        }
        return -1;
    }

    function currentType() {
        if (!state.doc || !state.selection || state.selection.typeIndex < 0) return null;
        var list = state.selection.kind === 'face' ? state.doc.types.faceTypes
            : state.selection.kind === 'edge' ? state.doc.types.edgeTypes
            : state.doc.types.vertexTypes;
        return list[state.selection.typeIndex] || null;
    }

    function fillSelect() {
        typeSelect.innerHTML = '';
        var placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.textContent = 'Choose a type';
        typeSelect.appendChild(placeholder);
        [
            ['Faces', 'face', state.doc.types.faceTypes],
            ['Edges', 'edge', state.doc.types.edgeTypes],
            ['Vertices', 'vertex', state.doc.types.vertexTypes],
        ].forEach(function (entry) {
            var group = document.createElement('optgroup');
            group.label = entry[0];
            (entry[2] || []).forEach(function (_, index) {
                var count = usageCount(entry[1], index);
                var option = document.createElement('option');
                option.value = entry[1] + ':' + index;
                option.textContent = index + ' · ' + count + ' ' + kindNoun(entry[1], count);
                group.appendChild(option);
            });
            typeSelect.appendChild(group);
        });
        typeSelect.value = state.selection && !isDecorationSelection()
            ? state.selection.kind + ':' + state.selection.typeIndex
            : '';
    }

    // A decoration is not part of a type. Opening one selects it in place of the type.
    function isDecorationSelection() {
        return !!state.selection && state.selection.kind === 'decoration';
    }

    function openDecoration(kind, id) {
        state.selection = { kind: 'decoration', decorationKind: kind, id: id, typeIndex: -1, elementIndex: -1 };
        state.hover = null;
        recordSelection();
        refreshPanel();
        draw();
    }

    function renameDecorationInHistory(kind, oldId, newId) {
        function update(entry) {
            if (entry && entry.kind === 'decoration' && entry.decorationKind === kind && entry.id === oldId) {
                entry.id = newId;
            }
        }
        update(state.selection);
        if (selectionHistory) selectionHistory.stack.forEach(update);
    }

    function updateSummary() {
        // A decoration is not a type, so the type picker and summary are not shown for it.
        typePicker.hidden = isDecorationSelection();
        summary.hidden = isDecorationSelection();
        if (isDecorationSelection()) return;
        if (!state.selection || !currentType()) {
            summary.textContent = 'Click a face, edge, or vertex.';
            return;
        }
        summary.textContent = kindLabel(state.selection.kind) + ' ' + state.selection.typeIndex;
    }

    function formatNum(n) {
        if (!Number.isFinite(n)) return '';
        return String(Math.round(n * 1e6) / 1e6);
    }

    function formatVec(v) {
        return formatNum(v.x) + ', ' + formatNum(v.y) + ', ' + formatNum(v.z);
    }

    function el(tag, className, text) {
        var node = document.createElement(tag);
        if (className) node.className = className;
        if (text != null) node.textContent = text;
        return node;
    }

    function addReadOnly(parent, label, text) {
        var row = el('p', 'readonly', label + ': ' + text);
        parent.appendChild(row);
    }

    function addText(parent, owner, key, label) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, label));
        var input = document.createElement('input');
        input.type = 'text';
        input.spellcheck = false;
        input.value = owner[key] == null ? '' : String(owner[key]);
        input.addEventListener('input', function () {
            owner[key] = input.value;
        });
        wrap.appendChild(input);
        parent.appendChild(wrap);
    }

    function addNumber(parent, owner, key, label, onChange) {
        var wrap = el('label', 'field');
        wrap.appendChild(el('span', null, label));
        var input = document.createElement('input');
        input.type = 'number';
        input.step = 'any';
        if (owner[key] != null && Number.isFinite(Number(owner[key]))) input.value = formatNum(Number(owner[key]));
        input.addEventListener('input', function () {
            var value = parseFloat(input.value);
            if (!Number.isFinite(value)) return;
            owner[key] = value;
            if (onChange) onChange();
        });
        wrap.appendChild(input);
        parent.appendChild(wrap);
        return input;
    }

    function addCheck(parent, owner, key, label) {
        var wrap = el('label', null);
        var input = document.createElement('input');
        input.type = 'checkbox';
        input.checked = !!owner[key];
        input.addEventListener('change', function () {
            owner[key] = input.checked;
        });
        wrap.appendChild(input);
        wrap.appendChild(document.createTextNode(label));
        parent.appendChild(wrap);
    }

    function addReadOnlyCheck(parent, label, value) {
        var checks = el('div', 'checks');
        var wrap = el('label', null);
        var input = document.createElement('input');
        input.type = 'checkbox';
        input.checked = !!value;
        input.disabled = true;
        wrap.appendChild(input);
        wrap.appendChild(document.createTextNode(label));
        checks.appendChild(wrap);
        parent.appendChild(checks);
    }

    function ensureVec(owner, key) {
        if (!owner[key] || typeof owner[key] !== 'object') owner[key] = { x: 0, y: 0, z: 0 };
        return owner[key];
    }

    function addVec3(parent, owner, key, label, onChange) {
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, label));
        var row = el('div', 'vec');
        var vec = owner[key] || { x: 0, y: 0, z: 0 };
        ['x', 'y', 'z'].forEach(function (axis) {
            var axisLabel = el('label', null, axis);
            var input = document.createElement('input');
            input.type = 'number';
            input.step = 'any';
            input.value = formatNum(Number(vec[axis]) || 0);
            input.addEventListener('input', function () {
                var value = parseFloat(input.value);
                if (!Number.isFinite(value)) return;
                ensureVec(owner, key)[axis] = value;
                if (onChange) onChange();
            });
            axisLabel.appendChild(input);
            row.appendChild(axisLabel);
        });
        wrap.appendChild(row);
        parent.appendChild(wrap);
    }

    function toHex(color) {
        function channel(v) {
            var n = Math.max(0, Math.min(255, Math.round((Number(v) || 0) * 255)));
            return n.toString(16).padStart(2, '0');
        }
        return '#' + channel(color.x) + channel(color.y) + channel(color.z);
    }

    function addColor(parent, owner) {
        var wrap = el('div', 'field');
        wrap.appendChild(el('span', null, 'Color'));
        var row = el('div', 'color-row');
        var picker = document.createElement('input');
        picker.type = 'color';
        var color = owner.color || { x: 0.8, y: 0.8, z: 0.8 };
        picker.value = toHex(color);
        var numbers = {};
        function write(next) {
            var stored = ensureVec(owner, 'color');
            stored.x = next.x;
            stored.y = next.y;
            stored.z = next.z;
            draw();
        }
        picker.addEventListener('input', function () {
            var hex = picker.value;
            var n = parseInt(hex.slice(1), 16);
            var next = { x: ((n >> 16) & 255) / 255, y: ((n >> 8) & 255) / 255, z: (n & 255) / 255 };
            ['x', 'y', 'z'].forEach(function (axis) { numbers[axis].value = formatNum(next[axis]); });
            write(next);
        });
        row.appendChild(picker);
        var vec = el('div', 'vec');
        ['x', 'y', 'z'].forEach(function (axis, index) {
            var axisLabel = el('label', null, ['r', 'g', 'b'][index]);
            var input = document.createElement('input');
            input.type = 'number';
            input.step = 'any';
            input.value = formatNum(Number(color[axis]) || 0);
            numbers[axis] = input;
            input.addEventListener('input', function () {
                var value = parseFloat(input.value);
                if (!Number.isFinite(value)) return;
                var stored = ensureVec(owner, 'color');
                stored[axis] = value;
                picker.value = toHex(stored);
                draw();
            });
            axisLabel.appendChild(input);
            vec.appendChild(axisLabel);
        });
        row.appendChild(vec);
        wrap.appendChild(row);
        parent.appendChild(wrap);
    }

    function extraKeys(owner, known) {
        var extra = {};
        var found = false;
        Object.keys(owner).forEach(function (key) {
            if (known.indexOf(key) >= 0) return;
            extra[key] = owner[key];
            found = true;
        });
        return found ? extra : null;
    }

    function addExtra(parent, owner, known) {
        var extra = extraKeys(owner, known);
        if (!extra) return;
        parent.appendChild(el('p', 'note', 'Other fields are kept in the file:'));
        parent.appendChild(el('pre', 'extra', JSON.stringify(extra, null, 2)));
    }

    function renderFaceFields(type) {
        if (hitIndex(state.selection, 'face') >= 0) {
            var face = state.view.faces[hitIndex(state.selection, 'face')];
            addReadOnly(fields, 'Mesh normal', formatVec(face.normal));
        }
        if (type.normal) addReadOnly(fields, 'Type normal', formatVec(type.normal));
        addText(fields, type, 'material', 'Material');
        addColor(fields, type);
        ['spacing', 'noiseScale', 'noiseIntensity', 'noiseDecay'].forEach(function (key) {
            if (type[key] != null) addNumber(fields, type, key, key);
        });
        addExtra(fields, type, ['material', 'color', 'normal', 'decoration', 'spacing', 'noiseScale', 'noiseIntensity', 'noiseDecay']);
    }

    // A table row that refers to another type. Hovering anywhere on the row
    // highlights that type on the canvas and clicking anywhere on it selects it.
    function typeRow(kind, typeIndex) {
        var row = el('tr', 'type-row');
        row.title = 'Select ' + kindLabel(kind).toLowerCase() + ' ' + typeIndex;
        row.addEventListener('mouseenter', function () {
            state.hover = { kind: kind, typeIndex: typeIndex, elementIndex: -1 };
            draw();
        });
        row.addEventListener('mouseleave', function () {
            if (!state.hover || state.hover.elementIndex >= 0) return;
            state.hover = null;
            draw();
        });
        row.addEventListener('click', function () {
            state.hover = null;
            selectType(kind, typeIndex);
        });
        return row;
    }

    function renderEdgeFields(type) {
        // Read-only first, then everything that can be edited.
        if (type.dir) addReadOnly(fields, 'Direction', formatVec(type.dir));
        addReadOnlyCheck(fields, 'Spliced', type.spliced);

        var faceData = el('fieldset');
        faceData.appendChild(el('legend', null, 'Face data'));
        var faceList = type.faceData || [];
        if (!faceList.length) {
            faceData.appendChild(el('p', 'note', 'No face data.'));
        } else {
            var table = el('table', 'half-table');
            var head = el('tr');
            ['#', 'Face type', 'Side'].forEach(function (name) {
                head.appendChild(el('th', null, name));
            });
            table.appendChild(head);
            faceList.forEach(function (entry, index) {
                var row = typeRow('face', Number(entry.type));
                row.appendChild(el('td', null, String(index)));
                row.appendChild(el('td', 'type-link', String(entry.type)));
                row.appendChild(el('td', null, entry.onRight ? 'right' : 'left'));
                table.appendChild(row);
            });
            faceData.appendChild(table);
        }
        fields.appendChild(faceData);
        // isRigidTiled is not read by the C++ import. edgeSettings["Rigid Tiled"] is the live setting.
        addExtra(fields, type, ['dir', 'isRigid', 'isRigidTiled', 'spliced', 'roundRadius', 'decoration', 'edgeSettings', 'faceData']);

        var checks = el('div', 'checks');
        addCheck(checks, type, 'isRigid', 'Rigid');
        fields.appendChild(checks);
        if (type.roundRadius != null) addNumber(fields, type, 'roundRadius', 'Round radius');

        var settings = el('fieldset');
        settings.appendChild(el('legend', null, 'Edge settings'));
        if (!type.edgeSettings) type.edgeSettings = {};
        addNumber(settings, type.edgeSettings, 'Min Length', 'Min length');
        addNumber(settings, type.edgeSettings, 'Max Length', 'Max length');
        addNumber(settings, type.edgeSettings, 'Tile Length', 'Tile length');
        var rigid = el('div', 'checks');
        addCheck(rigid, type.edgeSettings, 'Rigid Tiled', 'Rigid tiled');
        settings.appendChild(rigid);
        fields.appendChild(settings);
    }

    function renderVertexFields(type) {
        addReadOnlyCheck(fields, 'Spliced', type.spliced);

        var halfs = el('fieldset');
        halfs.appendChild(el('legend', null, 'Half-edge types'));
        var halfList = type.halfEdgeTypes || [];
        if (!halfList.length) {
            halfs.appendChild(el('p', 'note', 'No half-edge types.'));
        } else {
            var table = el('table', 'half-table');
            var head = el('tr');
            ['#', 'Edge type', 'Start', 'Direction'].forEach(function (name) {
                head.appendChild(el('th', null, name));
            });
            table.appendChild(head);
            halfList.forEach(function (half, index) {
                var row = typeRow('edge', Number(half.edge));
                row.appendChild(el('td', null, String(index)));
                row.appendChild(el('td', 'type-link', String(half.edge)));
                row.appendChild(el('td', null, half.isAtStart ? 'yes' : 'no'));
                row.appendChild(el('td', 'mono', half.dir ? formatVec(half.dir) : ''));
                table.appendChild(row);
            });
            halfs.appendChild(table);
        }
        fields.appendChild(halfs);
        addExtra(fields, type, ['spliced', 'desirability', 'decoration', 'halfEdgeTypes']);

        if (type.desirability == null) {
            var desire = el('label', 'field');
            desire.appendChild(el('span', null, 'Desirability'));
            var input = document.createElement('input');
            input.type = 'number';
            input.step = 'any';
            input.value = '0';
            input.addEventListener('input', function () {
                var value = parseFloat(input.value);
                if (!Number.isFinite(value)) return;
                type.desirability = value;
            });
            desire.appendChild(input);
            fields.appendChild(desire);
        } else {
            addNumber(fields, type, 'desirability', 'Desirability');
        }
    }

    function renderFields() {
        // Record any edit that has not been committed yet before the inputs are rebuilt.
        commitChange();
        fields.innerHTML = '';
        if (isDecorationSelection()) {
            window.PrimitivesDecorations.renderPanel(fields, state.selection.decorationKind, state.selection.id);
            absorbDefaults();
            updateHistoryButtons();
            return;
        }
        var type = currentType();
        if (!type) {
            updateHistoryButtons();
            return;
        }
        if (state.selection.kind === 'face') renderFaceFields(type);
        else if (state.selection.kind === 'edge') renderEdgeFields(type);
        else renderVertexFields(type);
        window.PrimitivesDecorations.render(fields, state.selection.kind, type);
        // Rendering can add missing defaults, such as edge settings. That is not an edit.
        absorbDefaults();
        updateHistoryButtons();
    }

    // ----- Undo and redo ------------------------------------------------
    // Two histories.
    //  - The edit history covers every change to the types and decorations. It is driven by
    //    Ctrl+Z, Ctrl+Y, and Shift+Ctrl+Z. Each entry is the JSON text of the
    //    types, so a restore never shares objects with the live document.
    //  - The selection history records which type was selected. The arrows at
    //    the top of the panel step back and forward through it.
    var whole = null;
    var selectionHistory = null;

    // The part of the document the editor changes: the types and the decorations.
    function editableText() {
        return JSON.stringify({ types: state.doc.types, decorations: state.doc.decorations });
    }

    function resetHistory() {
        whole = { stack: [editableText()], index: 0 };
        selectionHistory = { stack: [null], index: 0 };
    }

    function absorbDefaults() {
        if (!whole) return;
        whole.stack[whole.index] = editableText();
    }

    function pushWhole() {
        var text = editableText();
        if (text === whole.stack[whole.index]) return false;
        whole.stack.length = whole.index + 1;
        whole.stack.push(text);
        whole.index++;
        return true;
    }

    function commitChange() {
        if (!state.doc || !whole) return false;
        return pushWhole();
    }

    function restoreWhole(text) {
        var restored = JSON.parse(text);
        state.doc.types = restored.types;
        if (restored.decorations === undefined) delete state.doc.decorations;
        else state.doc.decorations = restored.decorations;
        refreshPanel();
        draw();
    }

    function undo() {
        if (!state.doc || !whole) return;
        commitChange();
        if (whole.index <= 0) return;
        whole.index--;
        restoreWhole(whole.stack[whole.index]);
    }

    function redo() {
        if (!state.doc || !whole) return;
        commitChange();
        if (whole.index + 1 >= whole.stack.length) return;
        whole.index++;
        restoreWhole(whole.stack[whole.index]);
    }

    function sameType(a, b) {
        if (!a || !b) return !a && !b;
        return a.kind === b.kind && a.typeIndex === b.typeIndex && a.id === b.id &&
            a.decorationKind === b.decorationKind;
    }

    function copySelection(selection) {
        return selection ? {
            kind: selection.kind,
            typeIndex: selection.typeIndex,
            elementIndex: selection.elementIndex,
            decorationKind: selection.decorationKind,
            id: selection.id,
        } : null;
    }

    // Call after state.selection changes because of something the user did.
    // Clicking another element of the type that is already selected is not a new step.
    function recordSelection() {
        if (!selectionHistory) return;
        var selection = copySelection(state.selection);
        if (sameType(selection, selectionHistory.stack[selectionHistory.index])) {
            selectionHistory.stack[selectionHistory.index] = selection;
            return;
        }
        selectionHistory.stack.length = selectionHistory.index + 1;
        selectionHistory.stack.push(selection);
        selectionHistory.index++;
    }

    function stepSelection(direction) {
        if (!state.doc || !selectionHistory) return;
        var target = selectionHistory.index + direction;
        if (target < 0 || target >= selectionHistory.stack.length) return;
        selectionHistory.index = target;
        state.selection = copySelection(selectionHistory.stack[target]);
        state.hover = null;
        refreshPanel();
        draw();
    }

    function updateHistoryButtons() {
        backBtn.disabled = !selectionHistory || selectionHistory.index <= 0;
        forwardBtn.disabled = !selectionHistory || selectionHistory.index + 1 >= selectionHistory.stack.length;
    }

    fields.addEventListener('change', function () { commitChange(); });
    backBtn.addEventListener('click', function () { stepSelection(-1); });
    forwardBtn.addEventListener('click', function () { stepSelection(1); });
    document.addEventListener('keydown', function (event) {
        if (!(event.ctrlKey || event.metaKey) || event.altKey) return;
        if (!state.doc || workspace.hidden) return;
        var key = event.key.toLowerCase();
        if (key === 'z' && !event.shiftKey) undo();
        else if (key === 'y' || (key === 'z' && event.shiftKey)) redo();
        else return;
        event.preventDefault();
    });

    function refreshPanel() {
        fillSelect();
        updateSummary();
        renderFields();
    }

    function updateHover(event) {
        if (!state.view || workspace.hidden) return;
        var rect = canvas.getBoundingClientRect();
        var hit = pick(event.clientX - rect.left, event.clientY - rect.top);
        if (sameHit(hit, state.hover)) return;
        state.hover = hit;
        draw();
    }

    function selectType(kind, typeIndex) {
        var list = kind === 'face' ? state.doc.types.faceTypes
            : kind === 'edge' ? state.doc.types.edgeTypes
            : state.doc.types.vertexTypes;
        if (!list || !list[typeIndex]) return;
        state.selection = {
            kind: kind,
            typeIndex: typeIndex,
            elementIndex: firstIndex(kind, typeIndex),
        };
        recordSelection();
        refreshPanel();
        draw();
    }

    function selectHit(hit) {
        if (!hit) {
            if (!state.selection) return;
            state.selection = null;
            recordSelection();
            refreshPanel();
            draw();
            return;
        }
        if (hit.typeIndex < 0) return;
        state.selection = hit;
        recordSelection();
        refreshPanel();
        draw();
    }

    function frameMesh(view) {
        var min = { x: Infinity, y: Infinity, z: Infinity };
        var max = { x: -Infinity, y: -Infinity, z: -Infinity };
        view.vertices.forEach(function (v) {
            min.x = Math.min(min.x, v.x);
            min.y = Math.min(min.y, v.y);
            min.z = Math.min(min.z, v.z);
            max.x = Math.max(max.x, v.x);
            max.y = Math.max(max.y, v.y);
            max.z = Math.max(max.z, v.z);
        });
        camera.target = {
            x: (min.x + max.x) / 2,
            y: (min.y + max.y) / 2,
            z: (min.z + max.z) / 2,
        };
        var extent = sub(max, min);
        camera.radius = Math.max(Math.sqrt(dot(extent, extent)) / 2, 0.5);
        camera.distance = camera.radius * 2.8;
        camera.theta = 0.7;
        camera.phi = 1.05;
    }

    function showLoadError(message) {
        loadError.hidden = false;
        loadError.textContent = message;
        workspace.hidden = true;
        empty.hidden = true;
        downloadBtn.disabled = true;
    }

    function loadText(text, fileName) {
        var doc;
        try {
            doc = JSON.parse(text);
        } catch (err) {
            showLoadError('Could not parse JSON: ' + err.message);
            return;
        }
        if (!doc || typeof doc.obj !== 'string') {
            showLoadError('This JSON has no embedded OBJ string in "obj".');
            return;
        }
        if (!doc.types) {
            showLoadError('This JSON has no "types".');
            return;
        }
        var loaded = window.PrimitivesMesh.loadMesh(doc.obj);
        if (loaded.error) {
            showLoadError('Could not read the embedded OBJ: ' + loaded.error);
            return;
        }
        var mismatch = window.PrimitivesMesh.countError(loaded.view, doc.types);
        state.doc = doc;
        resetHistory();
        state.fileName = fileName || 'primitives.json';
        state.view = loaded.view;
        state.selection = null;
        state.hover = null;
        loadError.hidden = true;
        empty.hidden = true;
        workspace.hidden = false;
        downloadBtn.disabled = false;
        if (mismatch) {
            matchWarning.hidden = false;
            matchWarning.textContent = mismatch + '. Clicks may not line up with the types in this file.';
        } else {
            matchWarning.hidden = true;
            matchWarning.textContent = '';
        }
        var types = doc.types;
        fileStatus.textContent = state.fileName + ' — ' +
            types.faceTypes.length + ' face types, ' +
            types.edgeTypes.length + ' edge types, ' +
            types.vertexTypes.length + ' vertex types';
        frameMesh(loaded.view);
        refreshPanel();
        requestAnimationFrame(function () {
            resize();
            draw();
        });
    }

    function download() {
        if (!state.doc) return;
        var blob = new Blob([JSON.stringify(state.doc, null, 2)], { type: 'application/json' });
        var url = URL.createObjectURL(blob);
        var link = document.createElement('a');
        link.href = url;
        link.download = state.fileName || 'primitives.json';
        link.click();
        URL.revokeObjectURL(url);
    }

    openBtn.addEventListener('click', function () { fileInput.click(); });
    downloadBtn.addEventListener('click', download);
    fileInput.addEventListener('change', function () {
        var file = fileInput.files && fileInput.files[0];
        fileInput.value = '';
        if (!file) return;
        var reader = new FileReader();
        reader.onload = function () { loadText(String(reader.result), file.name); };
        reader.onerror = function () { showLoadError('Could not read that file.'); };
        reader.readAsText(file);
    });

    typeSelect.addEventListener('change', function () {
        if (!state.doc) return;
        if (typeSelect.value === '') {
            state.selection = null;
        } else {
            var parts = typeSelect.value.split(':');
            var kind = parts[0];
            var typeIndex = Number(parts[1]);
            state.selection = {
                kind: kind,
                typeIndex: typeIndex,
                elementIndex: firstIndex(kind, typeIndex),
            };
        }
        recordSelection();
        updateSummary();
        renderFields();
        draw();
    });

    canvas.addEventListener('contextmenu', function (event) { event.preventDefault(); });
    canvas.addEventListener('pointerdown', function (event) {
        if (!state.view) return;
        canvas.setPointerCapture(event.pointerId);
        drag = { x: event.clientX, y: event.clientY, button: event.button, moved: false };
    });
    canvas.addEventListener('pointermove', function (event) {
        if (!drag) {
            updateHover(event);
            return;
        }
        var dx = event.clientX - drag.x;
        var dy = event.clientY - drag.y;
        if (Math.abs(dx) + Math.abs(dy) > 3) drag.moved = true;
        if (!drag.moved) return;
        if (state.hover) {
            state.hover = null;
        }
        if (drag.button === 2 || drag.button === 1 || event.shiftKey) {
            var frame = mvpMatrix();
            var backward = normalize(sub(frame.eye, camera.target));
            var xAxis = normalize(cross(UP, backward));
            var yAxis = cross(backward, xAxis);
            var panScale = camera.distance * 0.0022;
            camera.target = add(
                camera.target,
                add(scale(xAxis, -dx * panScale), scale(yAxis, dy * panScale))
            );
        } else {
            camera.theta += dx * 0.008;
            camera.phi = Math.max(0.08, Math.min(Math.PI - 0.08, camera.phi - dy * 0.008));
        }
        drag.x = event.clientX;
        drag.y = event.clientY;
        draw();
    });
    canvas.addEventListener('pointerup', function (event) {
        if (!drag) return;
        var moved = drag.moved;
        var button = drag.button;
        drag = null;
        if (moved || button !== 0 || !state.view) return;
        var rect = canvas.getBoundingClientRect();
        selectHit(pick(event.clientX - rect.left, event.clientY - rect.top));
    });
    canvas.addEventListener('pointerleave', function () {
        if (!state.hover) return;
        state.hover = null;
        draw();
    });
    canvas.addEventListener('wheel', function (event) {
        if (!state.view) return;
        event.preventDefault();
        var minDist = Math.max(camera.radius * 0.15, 0.05);
        var maxDist = camera.radius * 30;
        camera.distance = Math.max(minDist, Math.min(maxDist, camera.distance * Math.exp(event.deltaY * 0.001)));
        draw();
    }, { passive: false });

    window.addEventListener('resize', function () { draw(); });

    if (!gl) {
        showLoadError('WebGL is not available in this browser.');
    } else {
        try {
            initGl();
        } catch (err) {
            showLoadError('Could not start the viewer: ' + err.message);
            gl = null;
        }
    }

    window.primitivesEditor = {
        loadText: loadText,
        undo: undo,
        redo: redo,
        back: function () { stepSelection(-1); },
        forward: function () { stepSelection(1); },
        snapshot: function () {
            var type = currentType();
            return {
                fileName: state.fileName,
                kind: state.selection && state.selection.kind,
                elementIndex: state.selection ? state.selection.elementIndex : -1,
                typeIndex: state.selection ? state.selection.typeIndex : -1,
                hoverKind: state.hover && state.hover.kind,
                hoverIndex: state.hover ? state.hover.elementIndex : -1,
                summary: summary.textContent,
                material: type && type.material,
                faceCount: state.view && state.view.faces.length,
                warning: matchWarning.textContent,
            };
        },
    };
})();
