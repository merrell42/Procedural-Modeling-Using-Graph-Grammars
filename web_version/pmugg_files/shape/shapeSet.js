// The shape set handle all the geometric operations.

ms.shapeSet = function(opt_vertices, opt_edges, opt_group) {
	this.vertices = opt_vertices || [];
	this.edges = opt_edges || [];
	this.group = opt_group;
	this.group && this.group.addShape(this);
	this.key = ms.shapeSet.counter++;
};

ms.shapeSet.counter = 0;

ms.shapeSet.SNAP_DISTANCE = 8;
ms.shapeSet.NEAR_RADIUS = 12;
ms.shapeSet.NEAR_RADIUS2 = ms.shapeSet.NEAR_RADIUS * ms.shapeSet.NEAR_RADIUS;

ms.shapeSet.prototype.getGroup = function() {
	return this.group;
};

ms.shapeSet.prototype.setGroup = function(group) {
	this.group = group;
};

ms.shapeSet.prototype.isSnappable = function() {
	return true;
};

ms.shapeSet.prototype.printExample = function() {
	var vResult = [];
	for (var i = 0; i < this.vertices.length; i++) {
		var v = this.vertices[i].getPosition();
		vResult.push('[' + v.x +', ' + v.y + ']');
	}
	var eResult = [];
	for (var i = 0; i < this.edges.length; i++) {
		var e = this.edges[i];
		var startIndex = this.vertices.indexOf(e.start);
		var endIndex = this.vertices.indexOf(e.end);
		eResult.push('[' + startIndex +', ' + endIndex + ']');
	}
	return '{vertices: [' + vResult.join(', ') + '], edges: [' + eResult.join(', ') + ']}';
};

ms.shapeSet.prototype.create = function(a) {
	var vertices = [];
	for (var i = 0; i < a.vertices.length; i++) {
		var v = a.vertices[i];
		var vertex = new ms.exampleVertex(v[0], v[1]);
		vertices.push(vertex);
		this.vertices.push(vertex);
	}
	for (var i = 0; i < a.edges.length; i++) {
		var e = a.edges[i];
		this.edges.push(new ms.edge(vertices[e[0]], vertices[e[1]]));
	}
};

ms.shapeSet.prototype.copy = function(isLinked) {
	var group = isLinked ? this.group : new ms.shapeGroup();
	var result = new ms.shapeSet([], [], group);
	for (var i = 0; i < this.vertices.length; i++) {
		result.vertices.push(this.vertices[i].copy());
	}
	for (var i = 0; i < this.edges.length; i++) {
		var e = this.edges[i];
		var startIndex = this.vertices.indexOf(e.start.getVertex());
		var endIndex = this.vertices.indexOf(e.end.getVertex());
		var newEdge = new ms.edge(result.vertices[startIndex], result.vertices[endIndex]);
		newEdge.setBrush(e.getBrush());
		var startFace = e.start.getFace();
		var endFace = e.end.getFace();
		var startArea = startFace ? startFace.getArea() : null;
		var endArea =   endFace ? endFace.getArea() : null;
		newEdge.start.setFace(new ms.exampleFace(startArea));
		newEdge.end.setFace(new ms.exampleFace(endArea));
		result.addEdge(newEdge);
	}
	return result;
};

ms.shapeSet.prototype.getKey = function() {
	return this.key;
};

ms.shapeSet.prototype.select = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].getGroup().select();
	}
	for (var i = 0; i < this.edges.length; i++) {
		this.edges[i].select();
	}
};

ms.shapeSet.prototype.deselect = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].getGroup().deselect();
	}
	for (var i = 0; i < this.edges.length; i++) {
		this.edges[i].deselect();
	}
};

// HACK: This is not the right way to do this. Maybe move the
// selection stuff into transformed shape.
ms.shapeSet.prototype.getSelected = function() {
	if (this.vertices.length == 0) {
		return false;
	} else {
		return this.vertices[0].getGroup().getSelected()
	}
};

// Merge in a shape set. Do not snap together the vertices.
ms.shapeSet.prototype.addShape = function(shape) {
	this.vertices = this.vertices.concat(shape.vertices);
	this.edges = this.edges.concat(shape.edges);
};

// Merge two shape sets. Snap the new shape into place. Remove redundant vertices. Assume the edges are not redundant.
ms.shapeSet.prototype.snapIn = function(shape) {
	var vertices = shape.getVertices();
	var edges = shape.getEdges();
	for (var i = 0; i < vertices.length; i++) {
		var v = vertices[i];
		if (this.vertices.indexOf(v) < 0) {
			var isAdjacent = false;
			for (var j = 0; j < this.vertices.length; j++) {
				var vj = this.vertices[j];
				if (vj.distance(v) < ms.shapeSet.SNAP_DISTANCE) {
					isAdjacent = true;
					for (var k = 0; k < edges.length; k++) {
						var e = edges[k];
						var start = e.getStart();
						var end = e.getEnd();
						if (start.getGroup() == v) {
							start.setVertex(vj.getVertex());
						}
						if (end.getGroup() == v) {
							end.setVertex(vj.getVertex());
						}
					}
				}
			}
			if (!isAdjacent) {
				this.vertices.push(v);
			}
		}
	}
	this.edges = this.edges.concat(edges);
};

ms.shapeSet.prototype.addColor = function(color) {
	for (var i = 0; i < this.edges.length; i++) {
		this.edges[i].color = color;
	}
};

// Move all the vertices in the set without moving the group.
ms.shapeSet.prototype.moveSet = function(dx, dy) {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].getGroup().directMove(dx, dy);
	}
};

ms.shapeSet.prototype.makeUnique = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		var vertex = this.vertices[i];
		vertex.getGroup().makeUnique(vertex);
	}
};

// Move the selected vertices. Include those in the group.
ms.shapeSet.prototype.move = function(dx, dy) {
	for (var i = 0; i < this.vertices.length; i++) {
		var v = this.vertices[i];
		if (v.getGroup().getSelected()) {
			var vertices = this.group.getVerticesFromIndex(i);
			vertices.forEach(function(vertex) {
				vertex.getGroup().makeUnique(vertex);
				vertex.getGroup().directMove(dx, dy);
			});
		}
	}
};

ms.shapeSet.prototype.addVertex = function(v) {
	this.vertices.push(v);
};

ms.shapeSet.prototype.addEdge = function(e) {
	if (!e) {
		return;
	}
	if (e.arcLength() != 0) {
		this.edges.push(e);
		e.setShape(this);
	} else {
		e.removeMe();
	}
};

// Add a vertex connected by an edge to the previous vertex. Forms a
// simple polyline when repeatedly applied.
ms.shapeSet.prototype.addPolyLine = function(x, y) {
	var vertices = this.vertices;
	var prevVertex = vertices[vertices.length - 1];
	// var newVertex = new ms.point(x, y);
	var newVertex = new ms.exampleVertex(x, y);
	newVertex.getGroup().deselect();
	vertices.push(newVertex);
	var edge = new ms.edge(prevVertex, newVertex);
	edge.setBrush(ms.selector.getBrush());
	this.addEdge(edge);
};

ms.shapeSet.prototype.hover = function(v) {
	var changed = false;
	var vertices = this.getVertices();
	for (var i = 0; i < vertices.length; i++) {
		if (v == vertices[i]) {
			changed = changed || v.hover();
		} else {
			changed = changed || vertices[i].unhover();
		}
	}
	return changed;
};

ms.shapeSet.prototype.selectNone = function() {
	var vertices = this.getVertices();
	var edges = this.getEdges();
	for (var i = 0; i < edges.length; i++) {
		edges[i].deselect();
	}
};

ms.shapeSet.prototype.selectAll = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].getGroup().select();
	}
	for (var i = 0; i < this.edges.length; i++) {
		this.edges[i].getGroup().select();
	}
};

ms.shapeSet.prototype.freeze = function(freezeSelected) {};

ms.shapeSet.prototype.unfreezeAll = function() {};

ms.shapeSet.prototype.deleteSelected = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		if (this.vertices[i].getGroup().selected) {
			this.group.apply(function(shape) {
				shape.deleteVertex(i);
			});
			i--;
		}
	}
	for (var i = 0; i < this.edges.length; i++) {
		if (this.edges[i].selected) {
			this.deleteEdge(this.edges[i]);
			i--;
		}
	}
};

ms.shapeSet.prototype.deleteVertex = function(vertexIndex) {
	var v = this.vertices.splice(vertexIndex, 1)[0];
	var endpoints = v.getEndpoints().slice();
	for (var j = 0; j < endpoints.length; j++) {
		this.deleteEdge(endpoints[j].getEdge());
	}
};

ms.shapeSet.prototype.deleteEdge = function(e) {
	ms.maybeRemove(e, this.edges);
	e.removeMe();
};

ms.shapeSet.prototype.removeSelected = function() {
	var outsideVertices = [];
	var mixedVertices = [];
	var insideVertices = [];
	var oldVertices = this.vertices;
	for (var i = 0; i < this.vertices.length; i++) {
		var v = this.vertices[i];
		var endpoints = v.getEndpoints();
		var isUnselected = false;
		var isSelected = false;
		for (var j = 0; j < endpoints.length; j++) {
			if (endpoints[j].getEdge().getSelected()) {
				isSelected = true;
			} else {
				isUnselected = true;
			}
		}		
		if (isSelected && isUnselected) {
			mixedVertices.push(v);
		} else if (isSelected) {
			insideVertices.push(v);
		} else {
			outsideVertices.push(v);
		}
	}
	var newVertices = [];
	for(var i = 0; i < mixedVertices.length; i++) {
		newVertices.push(mixedVertices[i].copy());
	}
	var newEdges = [];
	var oldEdges = [];
	for (var i = 0; i < this.edges.length; i++) {
		var e = this.edges[i];
		if (e.getSelected()) {
			newEdges.push(e);			
			var startIndex = mixedVertices.indexOf(e.getStart().getVertex().getGroup());
			var endIndex   = mixedVertices.indexOf(  e.getEnd().getVertex().getGroup());
			if (startIndex >= 0) {
				var newVertex = newVertices[startIndex];
				e.getStart().setVertex(newVertex.getVertex());
			}
			if (endIndex >= 0) {
				var newVertex = newVertices[endIndex];
				e.getEnd().setVertex(newVertex.getVertex());
			}
		} else {
			oldEdges.push(e);
		}
	}
	this.edges = oldEdges;
	this.vertices = outsideVertices.concat(mixedVertices);
	newVertices = newVertices.concat(insideVertices);
	
	return new ms.shapeSet(newVertices, newEdges);
};

// TODO: Combine with selector.findVertex.
ms.shapeSet.prototype.findCloseVertex = function(query) {
	for (var i = 0; i < this.vertices.length; i++) {
		var v = this.vertices[i];
		if (query.distance2(v.getPosition()) < ms.shapeSet.NEAR_RADIUS2) {
			return v;
		}
	}
	return null;
};

ms.shapeSet.prototype.getEdges = function () {
	return this.edges;
};

// This includes the control points when the main points are selected.
ms.shapeSet.prototype.getVertices = function () {
	var result = [];
 	for (var i = 0; i < this.vertices.length; i++) {
		result = result.concat(this.vertices[i].getGroup().getVertices());
	}
	return result;
};

ms.shapeSet.prototype.getVertexIndex = function(vertex) {
	return this.vertices.indexOf(vertex);
};

ms.shapeSet.prototype.getVertexFromIndex = function(index) {
	return this.vertices[index];
};

ms.shapeSet.prototype.getSelectedVertices = function () {
	var result = [];
 	for (var i = 0; i < this.vertices.length; i++) {
		var v = this.vertices[i].getGroup();
		if (v.getSelected()) {
			result = result.concat(this.group.getVertexGroupsFromIndex(i));
		}
	}
	return result;
};

ms.shapeSet.prototype.getSelectedPositions = function () {
	var selectedVertices = this.getSelectedVertices();
	return selectedVertices.map(function(vertex) {
		return vertex.getPosition();
	});
	return positions;
};

ms.shapeSet.prototype.getConstraints = function () {
	var constraints = [];
	for (var i = 0; i < this.edges.length; i++) {
		constraints = constraints.concat(this.edges[i].getConstraints());
	}
	return constraints;
};

// Assumes that the shape is a closed polygon. Determine if a point is inside the polygon.
ms.shapeSet.prototype.isInside = function (v) {
	if (!v.getPosition()) {
		return false;
	}
	
	var leftCount = 0;
	for (var i = 0; i < this.edges.length; i++) {
		if (this.edges[i].isLeft(v)) {
			leftCount++;
		}
	}
	return leftCount % 2 == 1;
};

// Split this shape into two shapes. Assumes the shape is a
// simple 1D curve.
ms.shapeSet.prototype.split = function(index) {
	var vertices1 = this.vertices.slice(0, index + 1);
	var vertices2 = this.vertices.slice(index, this.vertices.length);
	var edges1 = this.edges.slice(0, index);
	var edges2 = this.edges.slice(index, this.edges.length);
	return [new ms.shapeSet(vertices1, edges1), new ms.shapeSet(vertices2, edges2)];
};

ms.shapeSet.tmpEdge = null;

ms.shapeSet.prototype.print = function() {
	for (var i = 0; i < this.edges.length; i++) {
		this.edges[i].print();
	}
};

ms.shapeSet.prototype.export = function(version, brushCollections) {
	var vertices = [];
	var vData = [];
	for (var i = 0; i < this.vertices.length; i++) {
		var vertex = this.vertices[i];
		var group = vertex.getGroup();
		vertices.push(vertex);
		var position = group.getPosition();
		
		var vDatum = {};
		var decoration = group.getDecoration();
		if (decoration && !decoration.isEmpty()) {
			vDatum = decoration.export();
		}
		vDatum.x = position.x;
		vDatum.y = position.y;
		vData.push(JSON.stringify(vDatum));
	}
	var controlPoints = [];
	var cPositions = [];
	var edgeIndices = [];
	var brushIndices = [];
	var areaIndices = [];
	for (var i = 0; i < this.edges.length; i++) {
		var indices = [];
		var e = this.edges[i];
		indices.push(vertices.indexOf(e.getStart().getVertex()));
		indices.push(vertices.indexOf(e.getEnd().getVertex()));
		indices.push(-1);
		indices.push(-1);
		edgeIndices.push(indices);
		brushIndices.push(brushCollections[0].getIndex(e.getBrush()));
		var startFace = e.getStart().getFace();
		var endFace =  e.getEnd().getFace();
		areaIndices.push(  endFace ? brushCollections[1].getIndex(  endFace.getArea()) : -1);
		areaIndices.push(startFace ? brushCollections[1].getIndex(startFace.getArea()) : -1);
	}
	return ['[' + vData.toString() + ']', cPositions.toString(), edgeIndices.toString(), brushIndices.toString(), areaIndices.toString()];
};

ms.shapeSet.import = function(version, exportedString, brushCollections, onChange) {
	var vData;
	if (version >= 5) {
		vData = JSON.parse(exportedString[0]);
	} else {
		vData = exportedString[0].split(',');
	}
	var cPositions = exportedString[1].split(',');
	var edgeIndices = exportedString[2].split(',');
	var brushIndices = (version >= 2) ? exportedString[3].split(',') : [];
	var areaIndices = (version >= 3) ? exportedString[4].split(',') : [];
	
	var shape = new ms.shapeSet();
	if (vData == '') {
		return shape;
	}
	var vertices = [];
	var scale = ms.globalSettings.get('Import Scale');
	var step = (version >= 5) ? 1 : 2;
	for (var i = 0; i < vData.length; i += step) {
		var v;
		if (version >= 5) {
			v = new ms.exampleVertex(scale * vData[i].x, scale * vData[i].y);
			if (vData[i].Image || vData[i].Desirability) {
				v.getGroup().setDecoration(ms.vertexDecoration.import(vData[i], onChange));
			}
		} else {
			v = new ms.exampleVertex(scale * parseFloat(vData[i]), scale * parseFloat(vData[i + 1]));
		}
		vertices.push(v);
		shape.addVertex(v);
	}
	
	for (var i = 0; i < edgeIndices.length; i += 4) {
		var s  = parseInt(edgeIndices[i]);
		var e  = parseInt(edgeIndices[i + 1]);
		var newEdge = new ms.edge(vertices[s], vertices[e]);
		if (version >= 2) {
			newEdge.setBrush(brushCollections[0].getFromIndex(parseInt(brushIndices[i / 4])));
		}
		if (version >= 3) {
			var getArea = function(index) {
				return index < 0 ? null : brushCollections[1].getFromIndex(index);
			}
			var area0 = getArea(parseInt(areaIndices[i / 2]));
			var area1 = getArea(parseInt(areaIndices[i / 2 + 1]))
			newEdge.getStart().setFace(new ms.exampleFace(area1));
			newEdge.getEnd().setFace(new ms.exampleFace(area0));
		}
		shape.addEdge(newEdge);
	}
	return shape; 
};

ms.shapeSet.prototype.smooth = function() {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices.smooth();		
	}
};

ms.shapeSet.prototype.lowerBound = function() {
	return ms.shapeSet.lowerBound(this.vertices);
};

ms.shapeSet.lowerBound = function(vertices) {
	var result = new ms.vec2(Infinity, Infinity);
	for (var i = 0; i < vertices.length; i++) {
		var pos = vertices[i].getPosition();
		result.x = Math.min(result.x, pos.x);
		result.y = Math.min(result.y, pos.y);
	}
	return result;
};

ms.shapeSet.prototype.upperBound = function() {
	return ms.shapeSet.upperBound(this.vertices);
};

ms.shapeSet.upperBound = function(vertices) {
	var result = new ms.vec2(-Infinity, -Infinity);
	for (var i = 0; i < vertices.length; i++) {
		var pos = vertices[i].getPosition();
		result.x = Math.max(result.x, pos.x);
		result.y = Math.max(result.y, pos.y);
	}
	return result;
};

// Find the nearest edge that is directly to the left of the query point.
ms.shapeSet.prototype.nearestLeftEdge = function(queryPosition) {
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].sortEndpoints();
	}

	var leftEdge = null;
	var isLeft;
	var xClose = -Infinity;
	for (var i = 0; i < this.edges.length; i++) {
		var e = this.edges[i];
		var intercepts = this.edges[i].intercept(queryPosition);
		var vertices = intercepts.vertices;
		for (var j = 0; j < vertices.length; j++) {
			var x = vertices[j].getPosition().x;
			if ((x > xClose) && (x < queryPosition.x - 1e-4)) {
				xClose = x;
				leftEdge = e;
				isLeft = (intercepts.tangents[j] > 0);
			}
		}
	}
	var endpoint = null;
	if (leftEdge) {
		endpoint = isLeft ? leftEdge.getEnd() : leftEdge.getStart();
	}
	return {edge: leftEdge, endpoint: endpoint, isLeft: isLeft, position: new ms.vec2(xClose, queryPosition.y)};
};

// Orient the polygons consistently.
ms.shapeSet.fixOrientation = function(vertexList) {
	if (ms.shapeSet.polygonArea(vertexList) > 0) {
		vertexList.reverse();
	}
};

ms.shapeSet.polygonArea = function(vertexList) {
	var sum = 0;
	var n = vertexList.length;
	for (var i = 0; i < n; i++) {
		var xi = vertexList[i][0];
		var yp = vertexList[(i + 1) % n][1];
		var yn = vertexList[(i + n - 1) % n][1];
		sum += xi * (yn - yp);
	}
	return sum / 2;
};

ms.shapeSet.faceArea = function(endpoints) {
	var sum = 0;
	var n = endpoints.length;
	for (var i = 0; i < n; i++) {
		var xi = endpoints[i].getVertex().getPosition().x;
		var yp = endpoints[(i + 1) % n].getVertex().getPosition().y;
		var yn = endpoints[(i + n - 1) % n].getVertex().getPosition().y;
		sum += xi * (yn - yp);
	}
	return -sum / 2;
}

// Due to dropping on the dimensions faceArea3D is not exactly equal to the area, but is proportional I think.
ms.shapeSet.faceArea3D = function(positions, maxDim) {
	var positions2D = positions.map((p) => { return p.dropDim(maxDim); });
	var sum = 0;
	var n = positions2D.length;
	for (var i = 0; i < n; i++) {
		var xi = positions2D[i].x;
		var yp = positions2D[(i + 1) % n].y;
		var yn = positions2D[(i + n - 1) % n].y;
		sum += xi * (yn - yp);
	}
	return -sum / 2;
}

ms.shapeSet.angleToQuadrant = function(angle) {
	if (angle < -Math.PI / 2) {
		return 3;				
	} else if (angle < 0) {
		return 4;
	} else if (angle < Math.PI / 2) {
		return 1;
	} else {
		return 2;
	}
};

ms.shapeSet.quadrantToOffset = function(quadrant, offset) {
	var WIDTH = 0.5;
	switch(quadrant) {
		case 1:  offset.x =  WIDTH; offset.y = -WIDTH;  break;
		case 2:  offset.x =  WIDTH; offset.y =  WIDTH;  break;
		case 3:  offset.x = -WIDTH; offset.y =  WIDTH;  break;
		case 4:  offset.x = -WIDTH; offset.y = -WIDTH;  break;
	}
};

ms.shapeSet.prototype.edgeSegments = function() {
	var segments = [];
	var edges = this.getEdges();
	var width = 0.5;
	for (var i = 0; i < edges.length; i++) {
		var segment = [];
		var v1 = edges[i].getStart().getVertex().getPosition();
		var v2 = edges[i].getEnd().getVertex().getPosition();
		var v = v2.copy()
		v.minus(v1);
		var vLength = v.length();
		v.normalize();
		var u = new ms.vec2(v.y, -v.x);
		u.scale(width / 4);
		v.scale(width);

		segment.push([v1.x + v.x + u.x, v1.y + v.y + u.y]);
		segment.push([v1.x + v.x - u.x, v1.y + v.y - u.y]);
		segment.push([v2.x - v.x - u.x, v2.y - v.y - u.y]);
		segment.push([v2.x - v.x + u.x, v2.y - v.y + u.y]);
		ms.shapeSet.fixOrientation(segment);
		segments.push(segment);
	}
	return segments;
};

ms.shapeSet.prototype.draw = function(view, offset, opt_color, secondPass) {
	var edges = this.getEdges();
	var vertices = this.getVertices();
	for (var i = 0; i < vertices.length; i++) {
		vertices[i].sortEndpoints();
	}

	// Draw edges.
	if (edges.length > 0) {
		view.context.lineWidth = 3;
		for (var i = 0; i < edges.length; i++) {
			view.context.beginPath();
			edges[i].draw(view, offset, opt_color, secondPass);
			view.context.stroke();
		}
	}
};