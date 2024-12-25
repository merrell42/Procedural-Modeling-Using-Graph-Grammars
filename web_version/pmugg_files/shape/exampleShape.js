// The example shape is the model of the MVC.

ms.exampleShape = function() {
	this.observers = [];
	this.reset();
	this.solution = null;
};

ms.exampleShape.prototype.reset = function() {
	// The first shape is always flexible.
	var shapeGroup = new ms.shapeGroup();
	var flexibleShape = new ms.transformedShape(new ms.shapeSet([], [], shapeGroup), null, false);
	this.shapes = [flexibleShape];
	this.faces = [];
	this.selectedShape = null;
};

ms.exampleShape.prototype.setSolution = function(solution) {
	this.solution = solution;
};


ms.exampleShape.prototype.isSnappable = function() {
	return true;
};

ms.exampleShape.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.EXAMPLE_SHAPE;
};

ms.exampleShape.blankArea = new ms.area('#fff', '#fff');

ms.exampleShape.export = function() {
	return window.controller.modeControllers[1].shapeMaker.exampleShape.export();
};

ms.exampleShape.prototype.import = function(version, data, brushCollections, onChange) {
	data = data.slice();
	var numUniqueShapes = data.shift();
	var uniqueShapes = [];
	for (var i = 0; i < numUniqueShapes; i++) {
		switch(version) {
			case 1:  shapeData = data.splice(0, 3); break;
			case 2:  shapeData = data.splice(0, 4); break;
			default: shapeData = data.splice(0, 5); break;
		}
		uniqueShapes.push(ms.shapeSet.import(version, shapeData, brushCollections, onChange));
	}
	var numTransformedShapes = data.shift();
	this.shapes = [];
	this.groups = [];
	for (var i = 0; i < numTransformedShapes; i++) {
		var group;
		if (version >= 3) {
			var groupIndex = data.shift();
			if (groupIndex > this.groups.length - 1) {
				this.groups.push(new ms.shapeGroup());
			}
			group = this.groups[groupIndex]
		} else {
			group = new ms.shapeGroup();
		}
		var shapeIndex = data.shift();
		var s = uniqueShapes[shapeIndex].copy();
		s.setGroup(group);
		group.addShape(s);
		this.shapes.push(ms.transformedShape.import(s, data));
	}

	// TODO: Move towards version 3. Convert to duplicate copies of shapes in shape groups.
	if (version >= 3) {
		var self = this;
		this.shapes.forEach(function(shape) {
			self.mergeVertexGroups(shape.getShape()); 
		});
	}
	this.updateFaces();
	this.selectedShape = this.shapes[0];
};

ms.exampleShape.prototype.setShape = function(shape) {
	this.shapes = [shape];
	this.faces = [];
	shape.parse();
};

ms.exampleShape.prototype.export = function(version, brushCollections) {
	var uniqueShapes = [];
	var uniqueGroups = [];
	var shapeIndices = [];
	var groupIndices = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var shape = this.shapes[i].getShape();
		var shapeIndex = uniqueShapes.indexOf(shape);
		if (shapeIndex < 0) {
			shapeIndex = uniqueShapes.length;
			uniqueShapes.push(shape);
		}
		shapeIndices.push(shapeIndex);
		
		var group = this.shapes[i].getShape().getGroup();
		var groupIndex = uniqueGroups.indexOf(group);
		if (groupIndex < 0) {
			groupIndex = uniqueGroups.length;
			uniqueGroups.push(group);
		}
		groupIndices.push(groupIndex);
	}
	var result = [uniqueShapes.length];
	for (var i = 0; i < uniqueShapes.length; i++) {
		result = result.concat(uniqueShapes[i].export(version, brushCollections));
	}
	result.push(this.shapes.length);
	for (var i = 0; i < this.shapes.length; i++) {
		if (version >= 3) {
			result.push(groupIndices[i]);
		}
		result.push(shapeIndices[i]);
		result = result.concat(this.shapes[i].export());
	}
	
	return result;
};

ms.exampleShape.prototype.print = function() {
	var keys = [];
	var shapeResult = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var key = this.shapes[i].getShape().key;
		if (keys.indexOf(key) == -1) {
			keys.push(key);
			shapeResult.push(this.shapes[i].getShape().printExample());
		}
	}
	var offsetResult = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var index = keys.indexOf(this.shapes[i].getShape().key);
		// The two zeros are for the offset just for backwards compatibility.
		offsetResult.push('[' + index + ', 0, 0]');
	}
	window.console.log('{shapes: [' + shapeResult.join(', ') + '], offsets: ['	+ offsetResult.join(', ') + ']}');
};

ms.exampleShape.prototype.create = function(x) {
	this.shapes = [];
	var shapes = [];
	for (var i = 0; i < x.shapes.length; i++) {
		var shape = new ms.shapeSet();
		shape.create(x.shapes[i]);
		shapes.push(shape);
	}
	for (var i = 0; i < x.offsets.length; i++) {
		var offset = x.offsets[i];
		var shape = shapes[offset[0]];
		var u = new ms.vec2(offset[1], offset[2]);
		this.shapes.push(new ms.transformedShape(shape, u));
	}
};

ms.exampleShape.print = function() {
	window.controller.modeControllers[ms.mainController.controllerTypes.EXAMPLE].exampleShape.print();
};

ms.exampleShape.prototype.getVertices = function () {
	if (this.selectedShape) {
		return this.selectedShape.getVertices();
	} else {
		return [];
	}
};

ms.exampleShape.prototype.getSelectedVertices = function () {
	var vertices = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var s = this.shapes[i];
		if (s.getSelected()) {
			vertices = vertices.concat(s.getVertices());
		}
	}
	return vertices;
};

ms.exampleShape.prototype.getAllVertices = function () {
	var vertices = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var s = this.shapes[i];
		vertices = vertices.concat(s.getVertices());
	}
	return vertices;
};

ms.exampleShape.prototype.getSelectedPositions = function () {
	var positions = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var s = this.shapes[i];
		if (s.getSelected()) {
			var vertices = s.getVertices();
			for (var j = 0; j < vertices.length; j++) {
				var v = vertices[j];
				var position = v.getPosition().copy();  // This is creating a lot of vertices. May use a lot of memory.
				positions.push(position);
			}
		}
	}
	return positions;
};

ms.exampleShape.prototype.getAllPositions = function () {
	var positions = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var s = this.shapes[i];
		var vertices = s.getVertices();
		for (var j = 0; j < vertices.length; j++) {
			var position = vertices[j].getPosition().copy();  // This is creating a lot of vertices. May use a lot of memory.
			positions.push(position);
		}
	}
	return positions;
};

ms.exampleShape.prototype.mergeVertexGroups = function (shape) {
	var movingPositions = shape.getSelectedPositions();
	var movingVertices = shape.getSelectedVertices();
	var allPositions = this.getAllPositions();
	var allVertices = this.getAllVertices();
	for (var i = 0; i < movingPositions.length; i++) {
		var a = movingPositions[i];
		for (var j = 0; j < allPositions.length; j++) {
			var b = allPositions[j];
			if (movingVertices[i] != allVertices[j]) {
				var dx = b.x - a.x;
				var dy = b.y - a.y;
				if (Math.abs(dx) + Math.abs(dy) < 1) {
					movingVertices[i].merge(allVertices[j]);
				}
			}
		}
	}
};

ms.exampleShape.prototype.getEdges = function () {
	var result = [];
	for (var i = 0; i < this.shapes.length; i++) {
		result = result.concat(this.shapes[i].getEdges());
	}
	return result;
};

ms.exampleShape.prototype.getEdgesWithKey = function (key) {
	var allEdges = this.getEdges();
	var edgesWithKey = [];
	for (var i = 0; i < allEdges.length; i++) {
		if (allEdges[i].getKey() == key) {
			edgesWithKey.push(allEdges[i]);
		}
	}
	return edgesWithKey;
};

ms.exampleShape.prototype.newShape = function () {
	this.selectedShape = null;
};

ms.exampleShape.prototype.addShape = function (s, isSelected, opt_isRigid) {
	var transformedShape = new ms.transformedShape(s, null, opt_isRigid);
	ms.transformedShape.counter++;
	this.shapes.push(transformedShape);
	if (isSelected) {
		this.selectedShape = transformedShape;
	}
	return transformedShape;
};

ms.exampleShape.prototype.selectNone = function() {
	for (var i = 0; i < this.shapes.length; i++) {
		this.shapes[i].deselect();
	}
};

ms.exampleShape.prototype.deleteSelected = function() {
	var newSet = [];
	for (var i = 0; i < this.shapes.length; i++) {
		if (!this.shapes[i].getSelected()) {
			newSet.push(this.shapes[i]);
		}
	}
	this.shapes = newSet;
};

ms.exampleShape.prototype.getShapes = function () {
	return this.shapes;
};

ms.exampleShape.prototype.getSelectedShape = function() {
	return this.selectedShape;
};

ms.exampleShape.prototype.setSelectedShape = function(selectedShape) {
	this.selectedShape = selectedShape;
};

ms.exampleShape.prototype.copySelected = function(isLinked) {
	var newShapes = [];
	for (var i = 0; i < this.shapes.length; i++) {
		var oldShape = this.shapes[i];
		if (oldShape.getSelected()) {
			var shape = oldShape.getShape().copy(isLinked);
			ms.transformedShape.counter++;
			shape.moveSet(100, 100);

			var newShape = new ms.transformedShape(shape, null);
			newShapes.push(newShape);

			newShape.select();
			oldShape.deselect();
		}
	}
	this.shapes = this.shapes.concat(newShapes);
};

ms.exampleShape.prototype.move = function(dx, dy) {
	for (var i = 0; i < this.shapes.length; i++) {
		if (this.shapes[i].getSelected()) {
			this.shapes[i].move(dx, dy);
		}
	}
};

ms.exampleShape.prototype.merge = function() {
	var primarySelected = null;
	for (var i = 0; i < this.shapes.length; i++) {
		if (this.shapes[i].getSelected()) {
			if (!primarySelected) {
				primarySelected = this.shapes[i];
			} else {
				primarySelected.snapIn(this.shapes[i]);
				this.shapes.splice(i, 1);
				i--;
			}
		}
	}
};

// Find the nearest edge that is directly to the left of the query point.
ms.exampleShape.prototype.nearestLeftEdge = function(queryPosition) {
	var nearest = {edge: null, endpoint: null, isLeft: false, position: new ms.vec2(-Infinity, queryPosition.y)};
	this.shapes.forEach(function(shape) {
		var newValue = shape.shape.nearestLeftEdge(queryPosition);
		if (newValue.position.x > nearest.position.x) {
			nearest = newValue;
		}
	});
	return nearest;
};

ms.exampleShape.prototype.updateFaces = function() {
	this.faces.forEach(function(face) {
		face.setDirty(true);
	});
	var newFaces = [];
	var updateEndpoint = function(endpoint) {
		var oldFace = endpoint.getFace();
		if (!oldFace || oldFace.isDirty()) {
			var area = oldFace ? oldFace.area : ms.exampleShape.blankArea;
			var newFace = new ms.exampleFace(area);
			newFace.addEndpoints(endpoint);
			newFaces.push(newFace);
		}
	};
	this.shapes.forEach(function(shape) {
		!shape.is3D && shape.getEdges().forEach(function(edge) {
			updateEndpoint(edge.getStart());
			updateEndpoint(edge.getEnd());
		});
	});
	
	this.faces = newFaces;
	this.faces.forEach(function(face) {
		face.setDirty(true);
	});
	var self = this;
	this.faces.forEach(function(face) {
		face.updateComponents(self);
	});
};

ms.exampleShape.prototype.draw = function(view, offset, secondPass) {
	if (!secondPass) {
		this.faces.forEach(function(face) {
			face.setDirty(true);
		});
		this.faces.forEach(function(face) {
			face.draw(view);
		});
	}

	for (var i = 0; i < this.shapes.length; i++) {
		var shape = this.shapes[i];
		if (shape.is3D) {
			continue;
		}
		var color = null;
		if (shape != this.selectedShape) {
			color = shape.getColor();
		}
		shape.draw(view, offset, color, secondPass);
	}
	for (var i = 0; i < this.shapes.length; i++) {
		var shape = this.shapes[i];
		if (shape.is3D) {
			this.draw3D(view, shape);
			continue;
		}
		var vertices = this.shapes[i].getVertices();
		for (var j = 0; j < vertices.length; j++) {
			vertices[j].draw(view, shape == this.selectedShape, secondPass);
		}
	}
};

ms.exampleShape.prototype.draw3D = function (view, shape) {
	if (!(view instanceof ms.view3d)) {
		return;
	}
	var model = shape.model;
	model.faces.forEach((face) => {
		const normal = model.vertexNormals[face.vertices[0].vertexNormalIndex];
		color =  [0.5 * normal.x + 0.5, 0.5 * normal.y + 0.5, 0.5 * normal.z + 0.5, 1];
		var corners = face.vertices.map((v) => {
			return model.vertices[v.vertexIndex];
		});
		if (corners.length == 4) {
			var c2 = corners[2];
			corners[2] = corners[3];
			corners[3] = c2;
			view.drawQuad(corners, color);
		} else if (corners.length == 3) {
			var corners3 = [
				corners[0].x, corners[0].y, corners[0].z,
				corners[1].x, corners[1].y, corners[1].z,
				corners[2].x, corners[2].y, corners[2].z,
			];
			view.drawTriangle(corners3, color);
		} else {
			var renderPositions = corners.flatMap((c) => [c.x, c.y, c.z]);
			var maxDim = ms.util.maxDim(normal);
			var allData = { positions: [] };
			var includeIndices = false;
			var dims = 3;
			ms.face.fillUsingEarcut(renderPositions, maxDim, allData, includeIndices, dims);
			for (var i = 0; i < allData.positions.length; i += 9) {
				view.drawTriangle(allData.positions.slice(i, i + 9), color);
			}
		}
	});
};

ms.exampleShape.prototype.isEmpty = function () {
	return !this.shapes.find(function(s) {
		return !s.isEmpty();
	})
};
