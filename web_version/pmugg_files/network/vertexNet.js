ms.vertexNet = function() {
	this.halfEdges = [];
	this.group = null;
	this.primal = null;
	this.network = null;

	this.id = ms.counter.add('vertexNet');
};

ms.counter.register('vertexNet');

ms.vertexNet.prototype.getHalfEdges = function() { return this.halfEdges; }
ms.vertexNet.prototype.getPrimal    = function() { return this.primal; }
ms.vertexNet.prototype.getNetwork   = function() { return this.network; }
ms.vertexNet.prototype.getId        = function() { return this.id; }

ms.vertexNet.prototype.setPrimal = function(primal) {
	this.primal = primal;
};

ms.vertexNet.prototype.connectNet = function(network) {
	this.network = network;
	network.addVertex(this);
	return this;
};

ms.vertexNet.prototype.setHalfEdge = function(halfEdge, index) {
	this.halfEdges[index] = halfEdge;
};

ms.vertexNet.prototype.copyConnection = function(copy) {
	var copyNet = copy.getNetwork();
	var self = this;
	this.halfEdges = copy.getHalfEdges().map((halfEdge) => {
		return self.network.convertHalfEdge(copyNet, halfEdge);
	});
	var copyGroup = copy.getGroup();
	this.group = copyGroup ?
		this.network.convertConnectorGroup(copyNet, copyGroup) :
		null;
};

ms.vertexNet.prototype.export = function() {
	var self = this;
	var group = this.getGroup();
	return {
		halfEdges: this.getHalfEdges().map((halfEdge) => {
			return self.network.halfEdgeIndex(halfEdge);
		}),
		group: group ? this.network.connectorGroupIndex(group) : -1,
	}
};

ms.vertexNet.prototype.import = function(json) {
	var self = this;
	this.halfEdges = json.halfEdges.map((halfEdge) => {
		return self.network.getHalfEdges()[halfEdge];
	});
	this.group = json.group >= 0 ?
		this.network.getConnectorGroups()[json.group] :
		null;
};

// This is just a sanity check. This should be true if this is valid.
ms.vertexNet.prototype.inNetwork = function() {
	return this.network.getVertices().includes(this);
};

ms.vertexNet.prototype.compare = function(endpointA, endpointB) {
	return ms.vertexType.compare(endpointA, endpointB);
};

ms.vertexNet.prototype.getGroup = function() {
	return this.group;
};

ms.vertexNet.prototype.setGroup = function(group) {
	this.group = group;
};

ms.vertexNet.prototype.highlight = function(view) {
	var options = ms.halfEdgeSet.create(this.halfEdges);
	this.network.highlight(view, options);
};

ms.vertexNet.prototype.print = function() {
	var interior = this.primal.getInterior();
	if (interior != this) {
		console.log ('Boundary Vertex');
		var halfEdge = interior.getHalfEdges()[0];
		if (halfEdge.getEdge()) {
			halfEdge.getEdge().print();
		} else {
			halfEdge.getPrev().getEdge().print();
		}		
	} else {
		ms.highlight(this);
	}
};

ms.vertexNet.prototype.requiresShapeView = function () {
	return true;
};