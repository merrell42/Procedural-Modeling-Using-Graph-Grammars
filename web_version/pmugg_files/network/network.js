ms.network = function() {
	this.vertices = [];
	this.edges = [];
	this.halfEdges = [];
	this.faces = [];
	this.connectorGroups = [];
	this.boundNet = null;

	this.id = ms.counter.add('network');
};

ms.counter.register('network');

ms.network.prototype.getVertices = function() {
	return this.vertices;
};

ms.network.prototype.getEdges = function() {
	return this.edges;
};

ms.network.prototype.getHalfEdges = function() {
	return this.halfEdges;
};

ms.network.prototype.getFaces = function() {
	return this.faces;
};

ms.network.prototype.getConnectorGroups = function() {
	return this.connectorGroups;
};

ms.network.prototype.getBoundNet = function() {
	return this.boundNet;
};

ms.network.prototype.addVertex = function(vertex) {
	this.vertices.push(vertex);
};

ms.network.prototype.addEdge = function(edge) {
	this.edges.push(edge);
};

ms.network.prototype.addHalfEdge = function(halfEdge) {
	this.halfEdges.push(halfEdge);
};

ms.network.prototype.addFace = function(face) {
	this.faces.push(face);
};

ms.network.prototype.addConnectorGroup = function(connectorGroup) {
	this.connectorGroups.push(connectorGroup);
};

ms.network.prototype.setBoundNet = function(boundNet) {
	this.boundNet = boundNet;
};

ms.network.prototype.isBoundary = function() {
	return this.boundNet.getBoundary() == this;
};

ms.network.prototype.isInterior = function() {
	return this.boundNet.getInterior() == this;
};

ms.network.prototype.removeVertex   = function(vertex)   {	ms.remove(vertex,   this.vertices); };
ms.network.prototype.removeEdge     = function(edge)     {	ms.remove(edge,     this.edges); };
ms.network.prototype.removeHalfEdge = function(halfEdge) {	ms.remove(halfEdge, this.halfEdges); };
ms.network.prototype.removeFace     = function(face)     {	ms.remove(face,     this.faces); };
ms.network.prototype.removeConnectorGroup = function(connectorGroup) {	ms.remove(connectorGroup, this.connectorGroups); };

// Convert a vertex to this network from a vertex in networkB.
ms.network.prototype.convertVertex = function(networkB, vertexB) {
	return vertexB && this.getVertices()[networkB.getVertices().indexOf(vertexB)];
};
ms.network.prototype.convertEdge = function(networkB, edgeB) {
	return edgeB && this.getEdges()[networkB.getEdges().indexOf(edgeB)];
};
ms.network.prototype.convertHalfEdge = function(networkB, halfEdgeB) {
	return halfEdgeB && this.getHalfEdges()[networkB.getHalfEdges().indexOf(halfEdgeB)];
};
ms.network.prototype.convertFace = function(networkB, faceB) {
	return faceB && this.getFaces()[networkB.getFaces().indexOf(faceB)];
};
ms.network.prototype.convertConnectorGroup = function(networkB, cGroupB) {
	return cGroupB && this.getConnectorGroups()[networkB.getConnectorGroups().indexOf(cGroupB)];
};

// Get the index of this a particular vertex;
ms.network.prototype.vertexIndex = function(vertex) {
	return vertex ? this.getVertices().indexOf(vertex) : -1;
};
ms.network.prototype.edgeIndex = function(edgeB) {
	return edgeB ? this.getEdges().indexOf(edgeB) : -1;
};
ms.network.prototype.halfEdgeIndex = function(halfEdgeB) {
	return halfEdgeB ? this.getHalfEdges().indexOf(halfEdgeB) : -1;
};
ms.network.prototype.faceIndex = function(faceB) {
	return faceB ? this.getFaces().indexOf(faceB) : -1;
};
ms.network.prototype.connectorGroupIndex = function(cGroupB) {
	return cGroupB ? this.getConnectorGroups().indexOf(cGroupB) : -1;
};

ms.network.prototype.copy = function() {
	var json = this.export();
	var result = ms.network.import(json);
	return result;
};

ms.network.prototype.export = function() {
	return {
		vertices:  this.vertices.map ((vertex)   => (vertex.export())),
		edges:     this.edges.map    ((edge)     => (edge.export())),
		halfEdges: this.halfEdges.map((halfEdge) => (halfEdge.export())),
		faces:     this.faces.map    ((face)     => (face.export())),
		connectorGroups: this.connectorGroups.map((cGroup) => (cGroup.export())),
	}
};

ms.network.import = function(json) {
	var result = new ms.network();
	json.vertices.forEach ((vertex)   => { (new ms.vertexNet()  ).connectNet(result); });
	json.edges.forEach    ((edge)     => { (new ms.edgeNet()    ).connectNet(result); });
	json.halfEdges.forEach((halfEdge) => { (new ms.halfEdgeNet()).connectNet(result); });
	json.faces.forEach    ((face)     => { (new ms.faceNet()    ).connectNet(result); });
	json.connectorGroups.forEach((cGroup) => { (new ms.connectorGroup()).connectNet(result); });

	json.vertices.forEach ((vertex  , index) => { result.getVertices()[index].import(vertex); });
	json.edges.forEach    ((edge    , index) => { result.getEdges()[index].import(edge); });
	json.halfEdges.forEach((halfEdge, index) => { result.getHalfEdges()[index].import(halfEdge); });
	json.faces.forEach    ((face    , index) => { result.getFaces()[index].import(face); });
	json.connectorGroups.forEach ((cGroup, index) => { result.getConnectorGroups()[index].import(cGroup); });

	return result;
};

ms.network.prototype.highlight = function(view, opt_options) {
	var options = opt_options || {};
	var size = ms.graph.HIGHLIGHTED_SIZE;
	var offset = options.offset || new ms.vec2(20, view.canvas.height - size - 20);
	options.rect = [offset.x, offset.x + size, offset.y, offset.y + size, 0, 100];
	this.draw(view, options);
};

ms.network.prototype.requiresShapeView = function () {
	return true;
};

// Assumes u and v are unit vectors.
ms.network.combineTangents = function(u, v) {
	if (u.dot(v) > 0.9) {
		return u;
	}
	var mathG = ms.fastMath;
	var uv = u.dot(v);
	var A = mathG.inv(mathG.matrix([[1, uv], [uv, 1]]));
	var B = mathG.multiply(A, mathG.matrix([[1], [1]]));
	var s = B.valueOf()[0][0];
	var t = B.valueOf()[1][0]
	return u.copy().scale(s).add(v.copy().scale(t));
};

/*
// I thought maybe computing n was the problem.
ms.network.combineHalfEdges = function(a, b) {
	var n = a.cross(b).normalize();
	var u = n.cross(a).normalize();
	var v = n.cross(b).normalize();
	return ms.network.combineTangents(u, v);
}; */

ms.network.prototype.draw = function(view, options) {
	var color = options.color || '#500';
	var rect = options.rect;
	var rectCenter = new ms.vec3((rect[0] + rect[1]) / 2, (rect[2] + rect[3]) / 2, (rect[4] + rect[5]) / 2);
	var rectExtents = new ms.vec3(rect[1] - rect[0], rect[3] - rect[2], rect[5] - rect[4]);
	if (rect.length != 6) {
		ms.alert('rect should be 3D.');
	}
	var scaleEdges = (rect[1] - rect[0]) / 60;
	var lineWidth = options.lineWidth || 2;

	// Return if the graph is empty.
	if (this.vertices.length == 0) {
		var r = rectExtents.x / 4;
		var black = [0, 0, 0, 1];
		view.drawCircle(rectCenter, r, 0.9 * r, black);
		var u = new ms.vec3(r, r, 0);
		var v = new ms.vec3(-0.05 * r, 0.05 * r, 0);
		var corners = [
			rectCenter.copy().add(u).add(v),
			rectCenter.copy().add(u).minus(v),
			rectCenter.copy().minus(u).add(v),
			rectCenter.copy().minus(u).minus(v),
		];
		view.drawQuad(corners, black);
		return;
	}
	var self = this;
	var vertexPositions = [];
	var vIndex = (v) => { return self.vertices.indexOf(v); };
	var getPosition = (v) => { return vertexPositions[vIndex(v)]; };
	var setPosition = (v, position) => { vertexPositions[vIndex(v)] = position; };
	
	var componentCount = 0;	
	var defaultLength = ms.graph.DEFAULT_LENGTH;
	var min = new ms.vec3(Infinity, Infinity, Infinity);
	var max = new ms.vec3(-Infinity, -Infinity, -Infinity);
	var updateMinMax = function(position) {
		min.x = Math.min(min.x, position.x);
		min.y = Math.min(min.y, position.y);
		min.z = Math.min(min.z, position.z);
		max.x = Math.max(max.x, position.x);
		max.y = Math.max(max.y, position.y);
		max.z = Math.max(max.z, position.z);
	};
	var componentVector = new ms.vec3(0, 0, 0);
	var verticesToAdd = this.vertices.slice();
	while (verticesToAdd.length > 0) {
		var startVertex = verticesToAdd.shift();
		setPosition(startVertex, componentVector.copy());

		// TODO: Progate backwards too using prev in addition to next to handle
		// some networks with only one half edge per face.
		var updateHalfEdges = startVertex.getHalfEdges().slice();
		updateMinMax(getPosition(startVertex));
		while (updateHalfEdges.length > 0) {
			var halfEdge = updateHalfEdges.shift();
			for (var sign = 0; sign < 2; sign++) {
				var oldVertex;
				var newVertex;
				var dir;
				if (sign == 0 && halfEdge.getNext()) {
					oldVertex = halfEdge.getVertex();
					newVertex = halfEdge.getNext().getVertex();
					dir = halfEdge.getDir().copy();
				} else if (sign == 1 && halfEdge.getPrev()) {
					oldVertex = halfEdge.getVertex();
					newVertex = halfEdge.getPrev().getVertex();
					dirSign = -1;
					dir = halfEdge.getPrev().getDir().copy().scale(-1);
				} else {
					continue
				}
				
				// TODO: Handle cycles properly.
				if (getPosition(newVertex)) {
					continue;
				}
				var baseLength = defaultLength;
				if (oldVertex.getPrimal().getBoundary() ||
					newVertex.getPrimal().getBoundary()) {
					baseLength = ms.graph.LEAF_LENGTH;
				}
				var prevPosition = getPosition(oldVertex).copy();
				var nextPosition;
				if (true) { // Edge lengths randomized or not.
					// var r = ms.util.originalRandom();
					var r = ms.util.random(this.id, verticesToAdd.length);
					nextPosition = prevPosition.add(dir.scale((1 + 0.75 * (r - 0.5)) * baseLength));
				} else {
					nextPosition = prevPosition.add(dir.scale(baseLength));
				}
				setPosition(newVertex, nextPosition);
				ms.remove(newVertex, verticesToAdd);
				var newHalfEdges = newVertex.getHalfEdges().slice();
				/* newHalfEdges = newHalfEdges.filter(function(halfEdgeB) {
					return halfEdge.edge != halfEdgeB.edge;
				}); */
				updateHalfEdges = updateHalfEdges.concat(newHalfEdges);
				updateMinMax(nextPosition);
			}
		}
		// Use the original random number generator to avoid changing the random number generator behavior during debugging.
		componentVector.x += ms.util.originalRandom() * ms.graph.COMPONENT_DISTANCE;
		componentVector.y += ms.util.originalRandom() * ms.graph.COMPONENT_DISTANCE;
	}
	
	var center = new ms.vec3((min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2);
	var extents = new ms.vec3(max.x - min.x, max.y - min.y, max.z - min.z);
	var scale = Math.min(rectExtents.x / extents.x, rectExtents.y / extents.y, rectExtents.z / extents.z) * (1 - ms.familyTree.MARGIN);

	var convertToScreen = function(v) {
		return new ms.vec3(
			scale * (v.x - center.x) + rectCenter.x,
			scale * (v.y - center.y) + rectCenter.y,
			scale * (v.z - center.z) + rectCenter.z
		);
	}

	var getStartEnd = function(half) {
		var start = convertToScreen(getPosition(half.getVertex()));
		var next = half.getNext();
		var end = next && convertToScreen(getPosition(next.getVertex()));
		return {start, end};
	};

	var faceEdgeWidth = ms.graph.FACE_EDGE_WIDTH;
	this.halfEdges.forEach(function(half) {
		var {start, end} = getStartEnd(half);
		var fDatum = half.getFaceDatum();
		var area = fDatum && fDatum.type.getMaterial();
		if (area && area.getColor && end) {
			view.drawEdge(start, end, area.getColor(), -1, faceEdgeWidth);
		}
	});

	this.edges.forEach(function(edge) {
		var brush = edge.getPrimal().getType().getBrush();
		var edgeColor = color
		if (brush && !options.colorOverride) {
			edgeColor = brush.getColor();
		}
		var {start, end} = getStartEnd(edge.getHalfEdges()[0][0]);
		view.drawEdge(start, end, edgeColor, 0, lineWidth * scaleEdges);
	});

	// Draw single-edge vertices that are not connectors.
	this.vertices.forEach(function(vertex) {
		if ((vertex.getHalfEdges().length == 1) && (!vertex.getPrimal().getBoundary())) {
			var position0 = getPosition(vertex);
			view.drawPoint(convertToScreen(position0), color);
		}
	});

	// TODO: Make this more efficient. I think the same half edges can be drawn
	// multiple times. Use the faceNets to make this more efficient.
	var w = scaleEdges * ms.graph.FACE_WIDTH_3D;
	var drawnHalfEdges = [];
	var allHalfEdges = this.getEdges().flatMap((e) => e.getHalfEdges().map((half) => half[0]));
	allHalfEdges.forEach(function(halfEdge0) {
		if (drawnHalfEdges.includes(halfEdge0)) {
			return;
		}
		// The graph draw function cycles over the face data. This shouldn't be necessary.
		// halfEdge0.getEdge().getPrimal().getType().getFaceData().forEach(function(faceDatum) {
			
		var faceHalfEdges = [];
		var faceDatum = halfEdge0.getFaceDatum();
		var type = faceDatum.type;
		var curHalfEdge = halfEdge0;
		
		// Follow the next halfEdge until it stops or it repeats.
		faceHalfEdges.push(curHalfEdge);
		curHalfEdge = curHalfEdge.getNext();
		while (curHalfEdge && curHalfEdge != halfEdge0) {
			var next = curHalfEdge.getNext();
			// Don't include the connectors that don't have a next.
			if (next) {
				faceHalfEdges.push(curHalfEdge);
			}
			curHalfEdge = next;
		}
		var looped = !!curHalfEdge;


		var tangents = faceHalfEdges.map(function(halfEdge) {
			return type.getNormal().cross(halfEdge.getDir()).normalize();
		});
		// The movement from the graph vertex to the extended face vertex.
		var movements = [];
		if (looped) {
			movements.push(ms.network.combineTangents(tangents[0], tangents[tangents.length - 1]).scale(w));
		} else {
			movements.push(tangents[0].copy().scale(w));
		}
		// Add two consective tangents.
		for (var i = 1; i < tangents.length; i++) {
			movements[i] = ms.network.combineTangents(tangents[i - 1], tangents[i]).scale(w);
		}
		if (!looped) {
			movements.push(tangents[tangents.length - 1].copy().scale(w));
		}

		var positions = faceHalfEdges.map(function(halfEdge) {
			return convertToScreen(getPosition(halfEdge.getVertex()));
		});
		if (!looped) {
			var pos = getPosition(faceHalfEdges[faceHalfEdges.length - 1].getNext().getVertex());
			positions.push(convertToScreen(pos));
		}
		var color = type.normalColor();
		var n = positions.length;
		for (var i = 0; i < n - 1; i++) {
			var corners = [
				positions[i].copy(),
				positions[i + 1].copy(),
				positions[i].copy().add(movements[i]),
				positions[i + 1].copy().add(movements[i + 1]),
			];
			view.drawQuad(corners, color);
		}
		if (looped) {
			var corners = [
				positions[0].copy(),
				positions[n - 1].copy(),
				positions[0].copy().add(movements[0]),
				positions[n - 1].copy().add(movements[n - 1]),
			];
			view.drawQuad(corners, color);
		}
		drawnHalfEdges = drawnHalfEdges.concat(faceHalfEdges);
	});

	var halfEdgeSet = options.halfEdgeSet;
	var halfEdges = halfEdgeSet ? halfEdgeSet.halfEdges : [];
	for (var i = 0; i < halfEdges.length; i++) {
		var halfEdge = halfEdges[i];
		var next = halfEdge.getNext();
		if (!next) {
			continue;
		}
		var startPos = getPosition(halfEdge.getVertex());
		var endPos = getPosition(next.getVertex());
		if (!startPos || !endPos) {
			continue;
		}
		start = convertToScreen(startPos);
		end = convertToScreen(endPos);
		if (halfEdgeSet.isBackwards && halfEdgeSet.isBackwards[i]) {
			[start, end] = [end, start];
		}
		view.drawArrow(start, end, '#f0f');
	}
};

ms.network.prototype.print = function() {
	ms.highlight(this);
	return this.id;
};
