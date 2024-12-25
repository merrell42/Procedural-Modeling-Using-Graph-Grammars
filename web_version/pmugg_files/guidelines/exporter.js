ms.exporter = function() {};

ms.exporter.activeAction = function(types, id) {
	return (types == 'all' || (JSON.parse(types)).includes(parseInt(id)));
};


ms.exporter.makeArray = function(x) {
	if (x === undefined) {
		return [];
	} else if (Array.isArray(x)) {
		return x;
	} else {
		return [x];
	}
};

ms.exporter.exportVec3 = function(v) {
	return '[' + v.toArray().toString() + ']';
};

// Split an array into an multiple arrays of length 3.
ms.exporter.split = function(array) {
	var result = [];
	for (var i = 0; i < array.length; i += 3) {
		result.push([array[i], array[i + 1], array[i + 2]]);
	}
	return '#([' + result.join('],[') + '])';
};

ms.exporter.exportEdge = function(type, position0, position1) {
	var exportVec3 = ms.exporter.exportVec3;
	var start = exportVec3(position0);
	var end = exportVec3(position1);
	
	var diff = position1.copy().minus(position0);
	var length = diff.length();
	var {angle, axis} = ms.vec3.angleAxis(ms.vec3.Z_HAT, diff);
	axis = exportVec3(axis);
	return {'@attributes': {type, start, end, axis, angle, length}};
};

ms.exporter.sliceFace = function(face, slice) {
	var spacing = JSON.parse(slice['@attributes'].spacing);
	var edgeType = JSON.parse(slice['@attributes'].edgeType);
	var dir = JSON.parse(slice['@attributes'].dir);
	dir = new ms.vec3(dir[0], dir[1], dir[2]);
	dir.normalize();
	var dMin = Infinity;
	var dMax = -Infinity;
	var positions = face.getEndpoints().map((endpoint) => {
		var pos = endpoint.getPosition();
		var d = dir.dot(pos);
		dMin = Math.min(d, dMin);
		dMax = Math.max(d, dMax);
		return pos;
	});
	var numSlices = Math.round((dMax - dMin) / spacing);
	if (numSlices == 0) {
		return [];
	}
	// s is the new spacing.
	var s = (dMax - dMin)  / numSlices;
	var sliceEnds = [];
	for (var i = 0; i < numSlices + 1; i++) {
		sliceEnds.push([]);
	}
	for (var i = 0; i < positions.length; i++) {
		var v0 = positions[i];
		var v1 = positions[(i + 1) % positions.length];
		
		var d0 = dir.dot(v0);
		var d1 = dir.dot(v1);
		if (d0 == d1) {
			continue;
		}
		if (d0 > d1) {
			[d1, d0] = [d0, d1];
			[v1, v0] = [v0, v1];
		}
		var a0 = (d0 - dMin) / s;
		var a1 = (d1 - dMin) / s;
		a0 = Math.ceil(a0 - 0.01);
		a1 = Math.floor(a1 + 0.01);
		for (var a = a0; a <= a1; a++) {
			var di = s * a + dMin;
			ti = (di - d0) / (d1 - d0);
			var sliceEnd = ms.vec3.lerp(v0, v1, ti);
			if (isNaN(sliceEnd.x)) {
				debugger;
			}
			sliceEnds[a].push(sliceEnd);
		}
	}

	var sliceEdges = [];
	for (var i = 0; i < numSlices + 1; i++) {
		if (sliceEnds[i].length != 2) {
			ms.alert('Slice Ends should have 2 endpoints, not ' + sliceEnds[i].length);
		} else {
			sliceEdges.push(ms.exporter.exportEdge(edgeType, sliceEnds[i][0], sliceEnds[i][1]));
		}
	}
	return sliceEdges;
};

ms.exporter.exportFace = function(face) {	
	var type = face.getFaceType().id;
	if (Number.isNaN(Number(type))) {
		type = face.getFaceType().idNum;
	}
	var faceData = {};
	var renderData = {getTextureData: () => { return faceData; }};
	face.fillRenderData(renderData, true);
	for (var i = 0; i < faceData.positions.length; i++) {
		faceData.positions[i] /= ms.view3d.GENERATED_SCALE;
	}
	// Maxscript is 1-indexed, not 0-indexed.
	for (var i = 0; i < faceData.triangleIndices.length; i++) {
		faceData.triangleIndices[i]++;
	}
	var vertices = ms.exporter.split(faceData.positions);
	var indices = ms.exporter.split(faceData.triangleIndices);
	return({'@attributes': {type, vertices, indices}});
};

ms.exporter.export = function(nodeStats, xml, viewport) {
	var exportVec3 = ms.exporter.exportVec3;
	var vertices = nodeStats.getElements('vertex').map((vertex) => {
		var position = vertex.getPosition();
		var pos = exportVec3(position);
		var type = vertex.getState().getType().id;
		return {'@attributes': {type, pos}};
	});
	
	// <edgeSegment type='12' start='[30,10,20]' end='[0,0,0]' axis="[1.0, 0, 0]" angle="45" length="10"/>
	var edges = nodeStats.getElements('line').map((line) => {
		var edgeType = line.getEdgeType();
		var type = edgeType.idNum || edgeType.getId();
		var endpoint0 = line.getEndpoints()[0];
		var position0 = endpoint0.getPosition();
		var position1 = endpoint0.next().getPosition();
		return ms.exporter.exportEdge(type, position0, position1);
	});
	
	// For the actual face.
	// <face type="5" vertices="#([0,0,0],[10,0,0],[0,10,0],[10,10,0])" indices="#([1,2,3],[2,4,3])"/>
	var faces = [];
	nodeStats.getElements('face').forEach((face) => {
		if (face.isHole()) {
			return;
		}
		var faceData = ms.exporter.exportFace(face);
		var holes = [];
		var group = face.getGroup();
		var groupFaces = group.getFaces();
		for(var i = 1; i < groupFaces.length; i++) {
			holes.push(ms.exporter.exportFace(groupFaces[i]));
		}
		if (holes.length > 0) {
			faceData.holes = holes;
		}
		faces.push(faceData);
	});

	// Extra decoration. Slice the faces into pieces.
	nodeStats.getElements('face').forEach((face) => {
		var id = face.getFaceType().id;
		console.log(id);
		ms.exporter.makeArray(xml.decoration.fDecoration).forEach((fDecoration) => {
			// TODO: Revert this.
			var id0 = id.split(',')[0];
			if (ms.exporter.activeAction(fDecoration['@attributes'].types, id0)) {
				var slices = ms.exporter.makeArray(fDecoration.slice);
				slices.forEach((slice) => {
					var sliceEdges = ms.exporter.sliceFace(face, slice);
					edges = edges.concat(sliceEdges);
				});
			};
		});
	});
	
	var eye = viewport.eye; //.copy().scale(ms.view3d.GENERATED_SCALE);
	var interest = viewport.getTarget(); // .copy().scale(ms.view3d.GENERATED_SCALE);
	var camera = {
		cameraData: {
			'@attributes': {
				eye: JSON.stringify(eye.toArray()),
				interest: JSON.stringify(interest.toArray()),
				up: JSON.stringify(viewport.up.toArray()),
				fov: JSON.stringify(viewport.fov),
			},
		},
	}
	var geometry = {
		geometry: {
			vertex: vertices,
			edgeSegment: edges,
			face: faces,
		}
	}
	return '<model scale=\'1\'>' + xml.decorationText + /* ms.util.objToXml(camera) + */ ms.util.objToXml(geometry) + '</model>';
};


ms.exporter.exportExample = function(data) {
	var prefix = '<model scale=\'0.1\'>' + data.decorationText;
	
	var edges = [];
	var faces = [];
	
	var exportVec3 = ms.exporter.exportVec3;
	var vertices = data.vertices.map((vertex) => {
		var pos = exportVec3(vertex.pos);
		var type = vertex.type;
		return {'@attributes': {type, pos}};
	});
	
	var edges = data.edges.map((edge) => {
		return ms.exporter.exportEdge(edge.type, edge.start, edge.end);
	});

	// This puts all the vertices into each face.
	// Obviously, this wastes a lot of space in the XML file.
	var vertexPositions = [];
	data.vertices.forEach((vertex) => {
		vertexPositions.push(vertex.pos.x);
		vertexPositions.push(vertex.pos.y);
		vertexPositions.push(vertex.pos.z);
	});
	vertexPositions = ms.exporter.split(vertexPositions);
	var faces = data.model.faces.map((face, index) => {
		var faceType = data.faceTypes[index];
		var id = faceType.id;
		var indices = face.vertices.map((vData) => {
			return vData.vertexIndex + 1;
		});
		indices = '#([' + indices + '])';

		var renderPositions = [];
		face.vertices.map((vData) => {
			var pos = data.vertices[vData.vertexIndex].pos;
			renderPositions.push(pos.x);
			renderPositions.push(pos.y);
			renderPositions.push(pos.z);
		});

		// Similar code is in ms.face.
		var maxDim = faceType.getMaxDim();
		var earcutPositions = [];
		for (var i = 0; i < renderPositions.length; i++) {
			if (i % 3 != maxDim) {
				earcutPositions.push(renderPositions[i]);
			}
		}
		var triangleIndices = earcut(earcutPositions, null, 2);
		// Maxscript is 1-indexed, not 0-indexed.
		for (var i = 0; i < triangleIndices.length; i++) {
			triangleIndices[i]++;
		}
		var vertices = ms.exporter.split(renderPositions);
		var indices = ms.exporter.split(triangleIndices);
		
		return {'@attributes': {type: id, vertices, indices}};
	});
	
	
	var geometry = {
		geometry: {
			vertex: vertices,
			edgeSegment: edges,
			face: faces,
		}
	};
	var geomXml = ms.util.objToXml(geometry);
	return prefix + geomXml + '\n</model>';
};