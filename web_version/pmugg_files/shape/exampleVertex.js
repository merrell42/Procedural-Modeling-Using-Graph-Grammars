// A vertex.
ms.exampleVertex = function(x, y) {
	this.endpoints = [];    // A list of edge endpoints.
	this.group = new ms.vertexGroup(x, y, this);

	// Just for debugging.
	this.id = ms.exampleVertex.counter++;
};

ms.exampleVertex.counter = 0;

ms.exampleVertex.prototype.getPosition = function() {
	return this.group.getPosition();
};

ms.exampleVertex.prototype.setGroup = function(group) {
	this.group = group;
};

ms.exampleVertex.prototype.getGroup = function(group) {
	return this.group;
};

ms.exampleVertex.prototype.getEdges = function() {
	var edges = [];
	for (var i = 0; i < this.endpoints.length; i++) {
		edges.push(this.endpoints[i].getEdge());
	}
	return edges;
};

ms.exampleVertex.prototype.getEndpoints = function() {
	return this.endpoints;
};

ms.exampleVertex.fromPosition = function(position) {
	var result = new ms.exampleVertex(position.x, position.y);
	return result;
};

ms.exampleVertex.prototype.copy = function() {
	var position = this.getPosition();
	var newVertex = new ms.exampleVertex(position.x, position.y);
	newVertex.getGroup().setDecoration(this.group.getDecoration());	
	return newVertex;
};

// Adds an endpoint.
ms.exampleVertex.prototype.addEndpoint = function(endpoint) {
	this.endpoints.push(endpoint);
};

ms.exampleVertex.prototype.sortEndpoints = function() {
	this.endpoints.sort(function(a, b) {
		return b.tangent() - a.tangent();
	});
};

// Removes an endpoint.
ms.exampleVertex.prototype.removeEndpoint = function(endpoint) {
	ms.remove(endpoint, this.endpoints);
};

ms.exampleVertex.prototype.move = function(dx, dy) {
	for (var i = 0; i < this.endpoints.length; i++) {
		this.endpoints[i].move(dx, dy);
	}
};

ms.exampleVertex.prototype.select = function() {
	for (var i = 0; i < this.endpoints.length; i++) {
		this.endpoints[i].select();
	}
};

ms.exampleVertex.prototype.getNext = function(endpoint) {
	return this.group.getNext(endpoint);
};
