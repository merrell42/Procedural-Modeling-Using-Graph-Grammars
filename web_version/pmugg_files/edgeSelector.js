// Selects edges and vertices of an object.

ms.edgeSelector = function(driver) {
	this.driver = driver;
};

ms.edgeSelector.prototype.select = function(selection, add, subtract) {
	// This is not generic. Only works with the shape maker.
	var selectedShape = this.driver.selectedShape();
	if (!selectedShape) {
		return;
	}
	var vertices = selectedShape ? selectedShape.getVertices() : [];
	var interiorVertices = [];
	for (var i = 0; i < vertices.length; i++) {
		var v = vertices[i];
		var isInside = selection.isInside(v);
		if (isInside) {
			interiorVertices.push(v);
		}
		ms.selector.updateSelectable(v, add, subtract, isInside);
	}

	// Selecting edges does nothing.
	/* var edges = selectedShape ? selectedShape.getEdges() : [];
	for (var i = 0; i < edges.length; i++) {
		var edgeI = edges[i];
		var intersection = ms.intersector.edgeShapeIntersect(edgeI, selection);
		var vStart = edgeI.getStart().getVertex().getGroup();
		var vEnd = edgeI.getEnd().getVertex().getGroup();
		intersection = intersection ||
				((interiorVertices.indexOf(vStart) != -1) &&
				 (interiorVertices.indexOf(vEnd) != -1));
		ms.selector.updateSelectable(edgeI, add, subtract, intersection);
	} */
};

// Find a selectable that is close to the position.
ms.edgeSelector.findClose = function(selectables, position, scale) {
	for (var i = 0; i < selectables.length; i++) {
		var selectable = selectables[i];
		if (selectable.selectType() == ms.shapeMaker.SelectableTypes.EXAMPLE_SHAPE) {
			var leftEdge = selectable.nearestLeftEdge(position);
			var endpoint = leftEdge.endpoint;
			var face = endpoint && endpoint.getFace();
			var area = face && face.getArea();
			if (area) {
				return face;
			}			
		} else {
			if (selectable.isClose(position, scale)) {
				return selectable;
			}
		}
	}
	return null;
}

// Return vertex near the position. Returns null if no vertices are close.
ms.edgeSelector.prototype.findVertex = function(position, opt_scale) {
	var scale = opt_scale || 1;
	if (!this.driver.selectedShape()) {
		return null;
	}
	return ms.edgeSelector.findClose(this.driver.getVertices(), position, scale);
};

ms.edgeSelector.prototype.pointSelect = function(position, options) {
	var add = options.add || false;
	var subtract = options.subtract || false;
	var scale = options.scale || 1;
	var selectables = this.driver.getSelectables(options);
	var selectable = ms.edgeSelector.findClose(selectables, position, scale);
	if (selectable) {
		if (!selectable.getSelected()) {
			if (!add) {
				this.driver.selectNone(false); // (v.isControl);
			}
			selectable.select();
		} else if (subtract) {
			selectable.deselect();
		}
	} else if (!add && !subtract) {
		this.driver.selectNone();
	}
	return selectable;
};
