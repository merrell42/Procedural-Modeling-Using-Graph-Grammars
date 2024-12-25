// Selects points

ms.selector = function(driver, internalSelector) {
	this.selection = null;
 	this.driver = driver;
	this.internalSelector = internalSelector;
	this.observers = [];
};

ms.selector.brush = null;

ms.selector.getBrush = function() {
	if (!ms.selector.brush) {
		ms.selector.brush = new ms.brush('#888', '#888', () => {});
	}
	return ms.selector.brush;
}

ms.selector.prototype.register = function(observer) {
	this.observers.push(observer);
};

ms.selector.prototype.remove = function(observer) {
	ms.remove(observer, this.observers);
};

ms.selector.prototype.notify = function() {
	for (var i = 0; i < this.observers.length; i++) {
		this.observers[i].redraw(this);
	}
};

ms.selector.prototype.getRenderables = function() {
	var renderables = this.driver.getRenderables().slice();
	if (this.selection) {
		renderables.push(this.selection);
	}
	return renderables;
};

ms.selector.prototype.getVertices = function () {
	return this.driver.getVertices();
};

ms.selector.prototype.getEdges = function () {
	var edges = this.driver.getEdges();
	if (this.selection) {
		edges = edges.concat(this.selection.getEdges());
	}
	return edges;
};

ms.selector.prototype.selectNone = function() {
	this.driver.selectNone();
	this.selection = null;
	this.notify();
};

ms.selector.prototype.selectAll = function() {
	this.driver.selectAll();
	this.selection = null;
	this.notify();
};

ms.selector.prototype.unfreezeAll = function() {
	this.driver.unfreezeAll();
	this.notify();
};

ms.selector.prototype.createSelectRect = function(event) {
	this.selection = new ms.shapeSet();
	for (var i = 0; i < 4; i++) {
		this.selection.vertices.push(new ms.point(event.x, event.y));
	}
	for (var i = 0; i < 4; i++) {
		var edge = new ms.edge(this.selection.vertices[i], this.selection.vertices[(i + 1) % 4]);
		edge.setBrush(ms.selector.getBrush());
		this.selection.edges.push(edge);
	}
	this.notify();
};

ms.selector.prototype.moveSelectRect = function(position) {
	if (!this.selection) {
		return;
	}
	var vertices = this.selection.vertices;
	vertices[1].position.x = position.x;
	vertices[2].position.x = position.x;
	vertices[2].position.y = position.y;
	vertices[3].position.y = position.y;
	this.notify();
};

ms.selector.prototype.createSelectLasso = function(position) {
	this.selection = new ms.shapeSet();
	var v = new ms.exampleVertex(position.x, position.y);
	this.selection.addVertex(v);
};

ms.selector.prototype.closeLasso = function(event) {
	if (!this.selection) {
		return;
	}
	var vertices = this.selection.vertices;
	var lastVertex = vertices[vertices.length - 1];
	var firstVertex = vertices[0];
	this.selection.edges.push(new ms.edge(lastVertex, firstVertex));
	this.notify();
};

ms.selector.prototype.addLasso = function(event) {
	if (!this.selection) {
		return;
	}
	this.selection.addPolyLine(event.x, event.y);
	this.notify();
};

ms.selector.prototype.select = function(add, subtract) {
	if (!this.selection) {
		return;
	}
	this.internalSelector.select(this.selection, add, subtract);
	this.selection = null;
	this.notify();
};

// Update an edge or vertex depending on if it was selected.
ms.selector.updateSelectable = function(selectable, add, subtract, insideSelection) {
	if (!add && !subtract) {
		if (insideSelection) {
			selectable.select();
		} else {
			selectable.deselect();
		}
	} else if (add && !selectable.getSelected() && insideSelection) {
		selectable.select();
	} else if (subtract && selectable.getSelected() && insideSelection) {
		selectable.deselect();
	}
};

ms.selector.prototype.findVertex = function(position, scale) {
	return this.internalSelector.findVertex(position, scale);
};

ms.selector.prototype.pointSelect = function(position, options) {
	var selectable = this.internalSelector.pointSelect(position, options);
	this.notify();
	return selectable;
};
