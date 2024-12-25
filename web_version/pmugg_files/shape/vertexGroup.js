// A vertex.
ms.vertexGroup = function(x, y, vertex) {
	this.position = new ms.vec2(x, y);
	this.vertices = [vertex];
	this.selected = false;
	this.visible = true;
	this.hovered = false;
	this.decoration = null;

	// Just for debugging.
	this.id = ms.vertexGroup.counter++;
};

ms.vertexGroup.prototype.isControl = function() {
	return false;
};

ms.vertexGroup.prototype.isSnappable = function() {
	return true;
};

ms.vertexGroup.prototype.getDecoration = function() {
	return this.decoration;
};

ms.vertexGroup.prototype.setDecoration = function(decoration) {
	this.decoration = decoration;
};

ms.vertexGroup.counter = 0;

ms.vertexGroup.prototype.merge = function(groupB) {
	this.vertices = this.vertices.concat(groupB.vertices);
	var self = this;
	groupB.vertices.forEach(function(vertex) {
		vertex.setGroup(self);
	});
	groupB.vertices = [];
	this.visible = true;
	this.selected = false;
	var decorationB = groupB.getDecoration();
	var imageB = decorationB && decorationB.get('Image');
	if (imageB && imageB != 'None') {
		this.decoration = groupB.getDecoration();
	}
};

ms.vertexGroup.create = function(p) {
	var result = new ms.vertexGroup(0, 0);
	result.position = p.getPosition();
	return result;
};

ms.vertexGroup.prototype.makeUnique = function(vertex) {
	if (this.vertices.length > 1) {
		this.removeVertex(vertex);
		vertex.setGroup(new ms.vertexGroup(this.position.x, this.position.y, vertex));
	}
};

ms.vertexGroup.prototype.removeVertex = function(vertex) {
	ms.remove(vertex, this.vertices);
};

ms.vertexGroup.prototype.getVertex = function() {
	if (this.vertices.length != 1) {
		alert('Wrong number of vertices in getVertex.');
	}
	return this.vertices[0];
};

ms.vertexGroup.fromPosition = function(position) {
	var result = new ms.vertexGroup(0, 0);
	result.position = position;
	return result;
};

ms.vertexGroup.prototype.getPosition = function() {
	return this.position;
};

ms.vertexGroup.prototype.setPosition = function(position) {
	this.position.set(position);
};

ms.vertexGroup.prototype.getSelectedPositions = function() {
	return [this.position];
};

ms.vertexGroup.prototype.getVertices = function () {
	return [this];
};

ms.vertexGroup.prototype.getSelectedVertices = function () {
	return [this];
};

ms.vertexGroup.prototype.getEdges = function() {
	var edges = [];
	for (var i = 0; i < this.vertices.length; i++) {
		edges = edges.concat(this.vertices[i].getEdges());
	}
	return edges;
};

ms.vertexGroup.prototype.getEndpoints = function() {
	var endpoints = [];
	for (var i = 0; i < this.vertices.length; i++) {
		endpoints = endpoints.concat(this.vertices[i].getEndpoints());
	}
	return endpoints;
};

ms.vertexGroup.prototype.getNext = function(endpoint) {
	var endpoints = this.getEndpoints();
	endpoints.sort(function(a, b) {
		return a.tangent() - b.tangent();
	});
	var index = endpoints.indexOf(endpoint);
	if (index == -1) {
		window.console.log('Endpoint not found');
		return null;
	}
	index = (index + 1) % endpoints.length;
	return endpoints[index];
};

ms.vertexGroup.prototype.isLarge =  function () {
	return true;
};

ms.vertexGroup.prototype.getColor =  function () {
	if (this.selected) {
		return ms.shapeView.SELECTED_COLOR;
	} else {
		if (this.hovered) {
			return ms.shapeView.HOVERED_COLOR;
		} else {
			return ms.shapeView.DEFAULT_COLOR;
		}
	}
};

ms.vertexGroup.prototype.getX = function() {
	return this.position.x;
};

ms.vertexGroup.prototype.getY = function() {
	return this.position.y;
};

ms.vertexGroup.prototype.distance = function(p) {
	return this.position.distance(p.getPosition());
};

ms.vertexGroup.prototype.sortEndpoints = function() {
	for (var j = 0; j < this.vertices.length; j++) {
		this.vertices[j].sortEndpoints();
	}
};

ms.vertexGroup.prototype.getSelected = function() {
	return this.selected;
};

ms.vertexGroup.prototype.hover = function() {
	var changed = !this.hovered;
	this.hovered = true;
	return changed;
};

ms.vertexGroup.prototype.unhover = function() {
	var changed = this.hovered;
	this.hovered = false;
	return changed;
};

ms.vertexGroup.prototype.select = function() {
	this.selected = true;
	for (var i = 0; i < this.vertices.length; i++) {
		this.vertices[i].select();
	}
};

ms.vertexGroup.prototype.deselect = function() {
	this.selected = false;
};

ms.vertexGroup.prototype.getVisible = function() {
	return this.visible;
};

ms.vertexGroup.prototype.copy = function() {
	var v = new ms.exampleVertex(this.position.x, this.position.y);
	return v.getGroup();
};

ms.vertexGroup.prototype.directMove = function(dx, dy) {
	this.position.x += dx;
	this.position.y += dy;
};

// TODO: Maybe use the direct move function more that doesn't search for
// hovered control points. This could speed up the exampleRepeater.
ms.vertexGroup.prototype.move = function(dx, dy) {
	if (this.selected) {
		this.position.x += dx;
		this.position.y += dy;
		for (var i = 0; i < this.vertices.length; i++) {
			this.vertices[i].move(dx, dy);
		}
	}
};

// Returns true is the position is close. This is used for selection.
ms.vertexGroup.prototype.isClose = function(query, scale) {
	var position = this.getPosition();
	return (position && query.distance2(position) < ms.shapeSet.NEAR_RADIUS2 / (scale * scale));
};

ms.vertexGroup.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.VERTEX;
};

ms.vertexGroup.prototype.getShape = function() {
	return this.vertices[0].endpoints[0].edge.shape;
};

ms.vertexGroup.prototype.draw = function(view, selected, secondPass) {
	view.drawVertex(this, selected, secondPass);
};
