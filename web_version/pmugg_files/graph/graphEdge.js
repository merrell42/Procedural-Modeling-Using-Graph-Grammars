ms.graphEdge = function(core) {
	this.core = core;
	this.endpoints = [null, null]; 
	this.graph = null;

	this.id = ms.graphEdge.count++;
};

ms.graphEdge.count = 0;

ms.graphEdge.prototype.export = function(types) {
	return {
		edge: types.edgeTypes.indexOf(this.core),
	}
};

ms.graphEdge.import = function(json, types) {
	var core = types.edgeTypes[json.edge];
	return new ms.graphEdge(core);
};

ms.graphEdge.prototype.setEndpoint = function(endpoint, index) {
	this.endpoints[index] = endpoint;
	endpoint && endpoint.setEdgeIndex(index);
};

ms.graphEdge.prototype.getEndpoint = function(index) {
	return this.endpoints[index];
};

ms.graphEdge.prototype.getVertex = function(index) {
	return this.endpoints[index].getVertex();
};

ms.graphEdge.prototype.getGraph = function() {
	return this.graph;
};

ms.graphEdge.prototype.setGraph = function(graph) {
	this.graph = graph;
};

ms.graphEdge.prototype.getCore = function() {
	return this.core;
};

ms.graphEdge.prototype.setCore = function(core) {
	this.core = core;
};

ms.graphEdge.prototype.getIndex = function() {
	return this.graph.getEdges().indexOf(this);
};

ms.graphEdge.prototype.equals = function(edgeB) {
	if (this.core != edgeB.core) {
		return false;
	}
	if (!this.endpoints[0].equals(edgeB.endpoints[0]) ||
		!this.endpoints[1].equals(edgeB.endpoints[1])) {
		return false;
	}
	return true;
};

ms.graphEdge.prototype.highlight = function(view) {
	var endpoints = this.endpoints.filter(function(e) {
		return !!e;
	});
	var options = {endpoints: endpoints};
	this.graph.highlight(view, options);
};

ms.graphEdge.prototype.requiresShapeView = function () {
	return true;
};

ms.graphEdge.prototype.print = function() {
	ms.highlight(this);
};
