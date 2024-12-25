// Selects shapes, not the edges and vertices.

ms.shapeSelector = function(driver) {
	this.driver = driver;
};

ms.shapeSelector.POINT_SELECTION_SIZE = 10;

// Return true if the shape is inside any part of the selection.
ms.shapeSelector.insideSelection = function(shape, selection) {
	var vertices = shape.getVertices();
	for (var i = 0; i < vertices.length; i++) {
		if (selection.isInside(vertices[i])) {
			return true;
		}
	}
	// Check the edges, if no vertices are inside the selection.
	var edges = shape.getEdges();
	for (var i = 0; i < edges.length; i++) {
		if (ms.intersector.edgeShapeIntersect(edges[i], selection)) {
			return true;
		}
	}
	return false;
};


ms.shapeSelector.prototype.select = function(selection, add, subtract) {
	var shapes = this.driver.getShapes();
	var anyInside = false;
	for (var j = 0; j < shapes.length; j++) {
		var shape = shapes[j];
		var isInside = ms.shapeSelector.insideSelection(shape, selection);
		anyInside = anyInside || isInside;
		ms.selector.updateSelectable(shape, add, subtract, isInside);
	}
	return anyInside;
};


ms.shapeSelector.prototype.findVertex = function(position) {
	return null;
};


ms.shapeSelector.prototype.pointSelect = function(position, options) {
	var add = options.add || false;
	var subtract = options.subtract || false;
	// scale doesn't do anything yet.
	var scale = options.scale || 1;

	var selection = new ms.shapeSet();
	var r = ms.shapeSelector.POINT_SELECTION_SIZE / 2;
	selection.vertices.push(new ms.point(position.x - r, position.y - r));
	selection.vertices.push(new ms.point(position.x - r, position.y + r));
	selection.vertices.push(new ms.point(position.x + r, position.y + r));
	selection.vertices.push(new ms.point(position.x + r, position.y - r));
	for (var i = 0; i < 4; i++) {
		selection.edges.push(new ms.edge(selection.vertices[i], selection.vertices[(i + 1) % 4]));
	}
	var shapes = this.driver.getShapes();
	for (var i = 0; i < shapes.length; i++) {
		var shape = shapes[i];
		if (ms.shapeSelector.insideSelection(shape, selection)) {
			// Deselect the rest only if shape is not selected.
			if (!shape.getSelected() && !add && !subtract) {
				for (var j = 0; j < shapes.length; j++) {
					if (shapes[j] != shape) {
						shapes[j].deselect();
					}
				}
			}
			if (!subtract) {
				shape.select();
			} else {
				shape.deselect();
			}
			return shape;
		}
	}
	if (!add && !subtract) {
		this.driver.selectNone();
	}
	return null;
};
