ms.shapeMaker = function(exampleShape, decorationModel) {
	this.exampleShape = exampleShape;
	this.decorationModel = decorationModel;
	this.decorationModel.addObserver(this);
	this.observers = [];

	this.prevVertex = null;
	this.newVertex = null;
	this.newEdge = null;
};

ms.shapeMaker.SelectableTypes = {
	VERTEX: 0,
	EDGE: 1,
	DRAGGABLE: 2,
	FACE: 3,
	EXAMPLE_SHAPE: 4,
};

ms.shapeMaker.export = function() {
	var array = window.controller.modeControllers[0].shapeMaker.export();
	var result = '[';
	for (var i = 0; i < array.length; i++) {
		if (typeof array[i] == 'string') {
			result = result + '\'' + array[i] + '\''
		} else {
			result = result + array[i].toString();
		}
		if (i < array.length - 1) {
			result = result + ', ';
		}
	}
	result = result + ']';
	console.dir(result);
	return result;
};

ms.shapeMaker.prototype.export = function() {
	var shape0 = this.exampleShape.shapes[0];
	if (shape0.is3D) {
		return [ms.testRunner.version3D, shape0.name];
	}
	var version = 5;
	var result = [version];
	for (var i = 0; i < 2; i++) {
		result = result.concat(this.decorationModel.brushCollections[i].export());
	}
	result = result.concat(this.exampleShape.export(version, this.decorationModel.brushCollections));
	return result;
};

ms.shapeMaker.exportGraphTemplate = function() {
	return window.controller.modeControllers[0].shapeMaker.exportGraphTemplate();
};

ms.shapeMaker.defaultEdgeColor = '#2896ff';

ms.shapeMaker.prototype.exportGraphTemplate = function() {
	var shape0 = this.exampleShape.shapes[0];
	if (shape0.is3D) {
		return '';
	}
	var shape = shape0.shape;
	var colorsToEdgeNum = {};
	var idToEdgeNum = {};
	var edgeCount = 0;
	shape.edges.forEach((edge) => {
		var color = edge.brush.getColor();
		if (color == ms.shapeMaker.defaultEdgeColor) {
			idToEdgeNum[edge.id] = edgeCount;
			edgeCount++;
		} else {
			var edgeNum = colorsToEdgeNum[color];
			if (edgeNum === undefined) {
				colorsToEdgeNum[color] = edgeCount;
				idToEdgeNum[edge.id] = edgeCount;
				edgeCount++;
			} else {
				idToEdgeNum[edge.id] = edgeNum;
			}
		}
	});

	var edgeIndices = [];
	shape.vertices.forEach((vertex) => {
		vertex.sortEndpoints();
		var endpoints = vertex.getEndpoints();
		if (endpoints.length == 1) {			
			var color = endpoints[0].edge.brush.getColor();
			if (color == ms.shapeMaker.defaultEdgeColor) {
				alert('Leaf is skipped with default edge color.');
			}
			return;
		}
		var edgeIndicesI = endpoints.map((endpoint) => (
			idToEdgeNum[endpoint.edge.id]
		));
		edgeIndices.push(edgeIndicesI);
	});
	var template = JSON.stringify(edgeIndices);
	// template = template.replaceAll('[','{').replaceAll(']','}');
	var brokenEdges = JSON.stringify(Object.values(colorsToEdgeNum));
	// brokenEdges = brokenEdges.replaceAll('[','{').replaceAll(']','}');
	return `{"comment": "", "numEdges": ${edgeCount}, "vertices": ${template}, "brokenEdges": ${brokenEdges}},`
};

ms.shapeMaker.prototype.register = function(observer) {
	this.observers.push(observer);
};

ms.shapeMaker.prototype.remove = function(observer) {
	ms.remove(observer, this.observers);
};

ms.shapeMaker.prototype.notify = function() {
	for (var i = 0; i < this.observers.length; i++) {
		this.observers[i].redraw(this);
	}
};

ms.shapeMaker.prototype.getRenderables = function() {
	var renderables = [];
	if (this.newEdge) {
		renderables.push(this.newEdge);
	}
	renderables.push(this.exampleShape);
	if (this.newVertex) {
		renderables.push(this.newVertex.getGroup());
	}
	renderables = renderables.concat(this.decorationModel.getRenderables());
	return renderables;
};

ms.shapeMaker.prototype.getVertices = function () {
	var result = this.exampleShape.getVertices().slice();
	if (this.newVertex) {
		result.push(this.newVertex.getGroup());
	}
	return result;
};

ms.shapeMaker.prototype.getEdges = function () {
	var result = this.exampleShape.getEdges().slice();
	if (this.newEdge) {
		result.push(this.newEdge);
	}
	return result;
};

ms.shapeMaker.prototype.getSelectables = function (options, position) {
	var types = ms.shapeMaker.SelectableTypes;
	var vertices = [];
	var draggables = [];
	var edges = [];
	var faces = [];

	if (options.types.includes(types.VERTEX)) {
		vertices = this.getVertices();
	}
	if (options.types.includes(types.DRAGGABLE)) {
		draggables = this.decorationModel.getRenderables();
	}
	if (options.types.includes(types.EDGE)) {
		edges = this.getEdges();
	}
	if (options.types.includes(types.FACE)) {		
		faces = [this.exampleShape];
	}
	
	// The order in which we select them is important. The draggables appear on top when a vertex is selected.
	var result;
	if (this.decorationModel.getVertex()) {
		result = draggables.concat(vertices);
	} else {
		result = vertices.concat(draggables);
	}
	result = result.concat(edges);
	result = result.concat(faces);
	return result;
};

ms.shapeMaker.prototype.selectedShape = function() {
	return this.exampleShape.getSelectedShape();
};

ms.shapeMaker.prototype.controlPointMove = function(dx, dy) {
	this.selectedShape().getShape().controlPointMove(dx, dy);
	this.notify();
};

ms.shapeMaker.prototype.updateNewVertex = function() {
	if (this.newVertex) {
		v = this.findCloseVertex();
		if (this.newEdge) {
			var endVertex = v ? v : this.newVertex;
			this.newEdge.getEnd().setVertex(endVertex);
		}
		this.newVertex.getGroup().visible = !v;
	}
	this.notify();
};

ms.shapeMaker.prototype.addVertex = function(v) {
	if (this.prevVertex) {
		var shape = this.selectedShape();
		if (!shape) {
			shape = new ms.shapeSet();
			this.exampleShape.addShape(shape, true);
		}
		this.newEdge = new ms.edge(this.prevVertex, v);
		this.newEdge.setBrush(this.decorationModel.getActiveBrush());
		this.newEdge.shape = shape;
	}
	this.notify();
	this.newVertex = v;
};

// Find a vertex close to the given vertex.
ms.shapeMaker.prototype.findCloseVertex = function() {	
	var v = null;		
	if (this.selectedShape()) {
		v = this.selectedShape().getShape().findCloseVertex(this.newVertex.getPosition());
	}
	return v;
}

// Place the new vertex down so it is no longer moving.
ms.shapeMaker.prototype.placeVertex = function() {
	var shape = this.selectedShape();
	if (!shape) {
		shape = this.exampleShape.addShape(new ms.shapeSet([], [], new ms.shapeGroup()), true, true);
	}
	shape = shape.getShape();
	var self = this;
	if (this.newVertex.getGroup().visible) {
		this.newVertex.getGroup().selected = false;
		this.prevVertex = this.newVertex;
		shape.addVertex(this.newVertex);
		shape.getGroup().apply(function(shapeB) {
			if (shapeB != shape) {
				var position = self.newVertex.getPosition().copy();
				position.add(shapeB.getVertexFromIndex(0).getPosition());
				position.minus(shape.getVertexFromIndex(0).getPosition());
				var vertexB = new ms.exampleVertex(position.x, position.y);
				shapeB.addVertex(vertexB);
			}
		});
		this.exampleShape.mergeVertexGroups(this.newVertex.getGroup());
	} else {
		if (this.newEdge) {
			this.prevVertex = null;
		} else {
			this.prevVertex = this.findCloseVertex();
		}
	}	
	if (this.newEdge) {
		var startIndex = shape.getVertexIndex(this.newEdge.getStart().getVertex());
		var endIndex = shape.getVertexIndex(this.newEdge.getEnd().getVertex());
		shape.getGroup().apply(function(shapeB) {
			var newEdge = new ms.edge(shapeB.getVertexFromIndex(startIndex),
			                          shapeB.getVertexFromIndex(endIndex));
			newEdge.setBrush(self.newEdge.getBrush());
			shapeB.addEdge(newEdge);
		});
		shape.deleteEdge(this.newEdge);
	}
	
	var position = this.newVertex.getPosition();
	var edgeToSplit = shape.getEdges().find(function(edge) {
		var coords = edge.edgeCoordinates(position);
		return (Math.abs(coords.v) < 1 && 0.005 < coords.u && coords.u < 0.995)
	});
	if (edgeToSplit) {
		edgeToSplit.split(this.newVertex);
		this.exampleShape.updateFaces();
	}

	this.newVertex = null;
	this.newEdge = null;
	this.notify();
};

ms.shapeMaker.prototype.addBoundary = function(brush, area) {
	var shape = this.selectedShape().getShape();
	// var cellWidth = ms.globalSettings.get('Major Grid Spacing');
	var v00 = new ms.exampleVertex(20, 20);
	var v01 = new ms.exampleVertex(20, 800);
	var v10 = new ms.exampleVertex(1560, 20);
	var v11 = new ms.exampleVertex(1560, 800);
	shape.addVertex(v00);
	shape.addVertex(v01);
	shape.addVertex(v10);
	shape.addVertex(v11);
	
	var addEdge = function(v0, v1) {
		var edge = new ms.edge(v0, v1);	
		edge.setBrush(brush);
		shape.addEdge(edge);
	};
	addEdge(v00, v01);
	addEdge(v01, v11);
	addEdge(v11, v10);
	addEdge(v10, v00);

	this.fillArea(new ms.vec2(9000, 400), area);
	this.notify();
}

ms.shapeMaker.prototype.getNewEdge = function() {
	return this.newEdge;
};

ms.shapeMaker.prototype.removeVertex = function() {
	this.newVertex = null;
	this.newEdge && this.newEdge.removeMe();
	this.newEdge = null;
	this.notify();
};

ms.shapeMaker.prototype.clearPrevVertex = function() {
	this.prevVertex = null;
};

ms.shapeMaker.prototype.selectNone = function(opt_controlOnly) {
	this.decorationModel.selectNone();
	if (this.selectedShape()) {
		this.selectedShape().getShape().selectNone(opt_controlOnly);
		this.notify();
	}
};

ms.shapeMaker.prototype.selectAll = function() {
	if (this.selectedShape()) {
		this.selectedShape().selectAll();
		this.notify();
	}
};

ms.shapeMaker.prototype.hover = function(v) {
	var changed = false;
	if (this.selectedShape()) {
		changed = changed || this.selectedShape().hover(v);
	}
	if (changed) {
		this.notify();
	}
};

ms.shapeMaker.prototype.newShape = function() {
	this.exampleShape.newShape();
	this.notify();
};

ms.shapeMaker.prototype.deleteSelected = function() {
	if (this.selectedShape()) {
		this.selectedShape().deleteSelected();
	}
	this.exampleShape.updateFaces();
	this.notify();
};

ms.shapeMaker.prototype.fillArea = function(position, area) {
	this.exampleShape.updateFaces();

	var leftEdge = this.exampleShape.nearestLeftEdge(position);
	var endpoint = leftEdge.endpoint;
	if (endpoint) {
		var oldFace = endpoint.getFace();
		if (oldFace) {
			if (oldFace.outerComponent) {
				oldFace.outerComponent.setArea(area);
			}
			oldFace.innerComponents.forEach(function(innerComponent) {
				innerComponent.setArea(area);
			});
			oldFace.setArea(area);
		}
	}
	this.notify();
};

ms.shapeMaker.prototype.split = function() {
	var originalShape = this.selectedShape().getShape();
	var splitShape = originalShape.removeSelected();
	splitShape = this.exampleShape.addShape(splitShape, true);
	this.notify();

	splitShape.select();
};

ms.shapeMaker.prototype.clear = function() {
	this.exampleShape.reset();
};

ms.shapeMaker.prototype.onCollectionChanged = function() {
	this.notify();
};
