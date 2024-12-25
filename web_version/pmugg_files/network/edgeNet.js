ms.edgeNet = function() {
	this.halfEdges = [];
	this.primal = null;
	this.network = null;

	this.id = ms.counter.add('edgeNet');
};

ms.counter.register('edgeNet');

ms.edgeNet.prototype.getHalfEdges = function() {
	return this.halfEdges;
};

ms.edgeNet.prototype.getPrimal    = function() { return this.primal; }
ms.edgeNet.prototype.getNetwork   = function() { return this.network; }

ms.edgeNet.prototype.setPrimal = function(primal) {
	this.primal = primal;
};

ms.edgeNet.prototype.connectNet = function(network) {
	this.network = network;
	network.addEdge(this);
	return this;
};

ms.edgeNet.prototype.addHalfEdge = function(halfEdge, index) {
	if (!this.halfEdges[index]) {
		this.halfEdges[index] = [];
	}
	this.halfEdges[index].push(halfEdge);
};

ms.edgeNet.prototype.removeHalfEdge = function(halfEdge, index) {
	ms.remove(halfEdge, this.halfEdges[index]);
};

ms.edgeNet.prototype.copyConnection = function(copy) {
	var copyNet = copy.getNetwork();
	var self = this;
	this.halfEdges = copy.getHalfEdges().map((halfEdges) => {
		return halfEdges.map((half) => {
			return self.network.convertHalfEdge(copyNet, half);
		});
	});
};

ms.edgeNet.prototype.export = function() {
	var self = this;
	return {	
		halfEdges: this.getHalfEdges().map((halfEdges) => {
			return halfEdges.map((half) => {
				return self.network.halfEdgeIndex(half);
			});
		}),
	}
};

ms.edgeNet.prototype.import = function(json) {
	var self = this;
	this.halfEdges = json.halfEdges.map((halfEdges) => {
		return halfEdges.map((half) => {
			return self.network.getHalfEdges()[half];
		});
	});
};

// This is just a sanity check. This should be true if this is valid.
ms.edgeNet.prototype.inNetwork = function() {
	return this.network.getEdges().includes(this);
};

ms.edgeNet.prototype.highlight = function(view) {
	var halfEdges = [];
	this.halfEdges.forEach((halfArray) => {
		halfEdges = halfEdges.concat(halfArray);
	});
	var options = ms.halfEdgeSet.create(halfEdges);
	this.network.highlight(view, options);
};

ms.edgeNet.prototype.print = function() {
	var interior = this.primal.getInterior();
	if (interior != this) {
		console.log ('Boundary Edge');
		interior.print();
	} else {
		ms.highlight(this);
	}
};

ms.edgeNet.prototype.requiresShapeView = function () {
	return true;
};

// Merge edgeB into this edge. Assumes this is an interior edge.
ms.edgeNet.prototype.merge = function(edgeB, mergeForward) {
	var interior = this.network;

	var halfsB = edgeB.getHalfEdges();
	this.halfEdges.forEach((half, edgeIndex) => {
		var halfA = half[0];
		var halfB = halfsB[edgeIndex][0];
		var forward = halfA.getForward();
		if (!(forward ^ mergeForward)) {
			var nextB = halfB.getNext();
			halfA.disconnectHalfEdge();
			halfB.disconnectHalfEdge();
			halfA.connectHalfEdge(nextB);
		} else {
			var prevB = halfB.getPrev();
			if (prevB) {
				prevB.disconnectHalfEdge();
				halfB.disconnectHalfEdge();
				prevB.connectHalfEdge(halfA);
			}
			halfA.connectVertex(halfB.getVertex(), halfA.getVertexIndex());
		}		
		var face = halfB.getFace();
		face.replaceHalfEdge(halfB, halfA);
		interior.removeHalfEdge(halfB);
	});
	interior.removeEdge(edgeB);
};
