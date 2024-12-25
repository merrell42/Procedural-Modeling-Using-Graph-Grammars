// An endpoint of an edge
ms.exampleEndpoint = function(edge, vertex, isAtStart) {
	this.edge = edge;
	this.vertex = vertex;
	this.face = null;
	this.isAtStart = isAtStart;
};

ms.exampleEndpoint.prototype.initialize = function() {
	this.vertex.addEndpoint(this);
};

ms.exampleEndpoint.prototype.getPosition = function() {
	return this.vertex.getPosition();
};

ms.exampleEndpoint.prototype.getVertex = function() {
	return this.vertex;
};

ms.exampleEndpoint.prototype.getGroup = function() {
	return this.vertex.getGroup();
};

ms.exampleEndpoint.prototype.getEdge = function() {
	return this.edge;
};

ms.exampleEndpoint.prototype.getFace = function() {
	return this.face;
};

ms.exampleEndpoint.prototype.setFace = function(face) {
	this.face = face;
};

ms.exampleEndpoint.prototype.removeMe = function() {
	this.vertex.removeEndpoint(this);
	this.vertex = null;
};

ms.exampleEndpoint.prototype.setVertex = function(v) {
	v.addEndpoint(this);
	if (this.vertex) {
		this.vertex.removeEndpoint(this);
	}
	this.vertex = v;
};

ms.exampleEndpoint.prototype.getVertices = function() {
	return [];
};

ms.exampleEndpoint.prototype.distance = function(endpoint) {
	return this.vertex.getPosition().distance(endpoint.vertex.getPosition());
};

// Move only the control point.
ms.exampleEndpoint.prototype.move = function(dx, dy) {};

ms.exampleEndpoint.prototype.select = function() {};

ms.exampleEndpoint.prototype.tangent = function() {
	var t = this.edge.getT(this);
	var tangent = this.edge.tangent(t);
	// Return the opposite tangent if we are at the end of the edge.
	if (t == 1) {
		if (tangent > 0) {
			tangent -= Math.PI;
		} else {
			tangent += Math.PI;
		}
	}
	return tangent;
};

ms.exampleEndpoint.prototype.direction = function() {
	var t = this.edge.getT(this);
	return this.edge.direction(t);
};

ms.exampleEndpoint.prototype.getNext = function() {
	return this.vertex.getNext(this);
};

ms.exampleEndpoint.prototype.twin = function() {
	return this.isAtStart ? this.edge.getEnd() : this.edge.getStart();
};

// This is not working.
ms.exampleEndpoint.prototype.draw = function(view, highlighted) {
	var vertex = this.vertex;
	if (vertex) {
		var color = highlighted ? '#044' : '#484'
		ms.endpoint.drawArrow(view.getContext(), vertex.getPosition(), -this.tangent(), color);
	}
};

// This is not working.
ms.exampleEndpoint.prototype.highlight = function(view) {
	this.draw(view, true);
};

// This is not working.
ms.exampleEndpoint.prototype.print = function() {
	ms.highlight(this);
};
