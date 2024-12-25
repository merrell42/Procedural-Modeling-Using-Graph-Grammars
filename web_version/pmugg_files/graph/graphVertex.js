ms.graphVertex = function(core) {
	this.graph = null;
	this.endpoints = [];
	this.core = core;
};

ms.graphVertex.prototype.export = function(types) {
	var result = {};
	if (this.isOuter()) {
		result.outer = true;
	} else {
		result.vertex = types.vertexTypes.indexOf(this.core);
	}
	return result;
};

ms.graphVertex.import = function(json, types) {
	var core = null;
	if (json.outer) {
		core = new ms.outerVertex();
	} else {
		core = types.vertexTypes[json.vertex];
	}
	return new ms.graphVertex(core);
};

ms.graphVertex.prototype.setCore = function(core) {
	this.core = core;
};

ms.graphVertex.prototype.getCore = function() {
	return this.core;
};

ms.graphVertex.prototype.setGraph = function(graph) {
	this.graph = graph;
};

ms.graphVertex.prototype.getGraph = function() {
	return this.graph;
};

ms.graphVertex.prototype.addEndpoint = function(endpoint) {
	var i = 0;
	for (i = 0; i < this.endpoints.length; i++) {
		if (this.core && this.core.compare(this.endpoints[i], endpoint)) {
			break;
		}
	}
	this.endpoints.splice(i, 0, endpoint);
	for (var j = i; j < this.endpoints.length; j++) {
		this.endpoints[j].setVertexIndex(j);
	}
};

ms.graphVertex.prototype.removeEdge = function(edge) {
	var endpoint = this.endpoints.find(function(endpoint) {
		return endpoint.edge == edge;
	});
	ms.remove(endpoint, this.endpoints);
	for (var j = 0; j < this.endpoints.length; j++) {
		this.endpoints[j].setVertexIndex(j);
	}
};

ms.graphVertex.prototype.isBoundary = function() {
	if (this.isOuter() || this.getCore().isPretty) {
		return false;
	}
	for (var i = 0; i < this.endpoints.length; i++) {
		var brush = this.endpoints[i].getEdge().getCore().getBrush();
		if (!brush || !brush.get('Boundary')) {
			return false;
		}
	}
	return true;
};

ms.graphVertex.prototype.isOuter = function() {
	return this.graph.getOuterVertex() == this;
};

ms.graphVertex.prototype.getEndpoints = function() {
	return this.endpoints;
};

ms.graphVertex.prototype.reorderEndpoints = function(newOrdering) {
	this.endpoints = ms.familyTreeEdge.reorder(this.endpoints, newOrdering);
};

ms.graphVertex.prototype.equals = function(vertexB) {
	if (this.isOuter() != vertexB.isOuter()) {
		return false;
	}
	if (this.isOuter()) {
		return true;
	}
	if (this.core != vertexB.core) {
		return false;
	}
	if (this.endpoints.length != vertexB.endpoints.length) {
		return false;
	}
	for (var i = 0; i < this.endpoints.length; i++) {
		if (!this.endpoints[i].equals(vertexB.endpoints[i])) {
			return false;
		}
	}
	return true;
};

ms.graphVertex.prototype.highlight = function(view) {
	var options = {endpoints: this.endpoints};
	this.getGraph().highlight(view, options);
};

ms.graphVertex.prototype.requiresShapeView = function () {
	return true;
};

ms.graphVertex.prototype.print = function() {
	ms.highlight(this);
};
