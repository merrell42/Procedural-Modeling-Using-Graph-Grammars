ms.halfEdgeNet = function(forward) {
	this.forward = forward;
	this.vertex = null;
	this.edge = null;
	this.vertexIndex = -1;
	this.edgeIndex = -1;
	this.prev = null;
	this.next = null;
	this.face = null;
	this.network = null;

	this.id = ms.counter.add('halfEdgeNet');
};

ms.counter.register('halfEdgeNet');

ms.halfEdgeNet.prototype.getForward = function() { return this.forward; }
ms.halfEdgeNet.prototype.getVertex  = function() { return this.vertex; }
ms.halfEdgeNet.prototype.getEdge    = function() { return this.edge; }
ms.halfEdgeNet.prototype.getPrev    = function() { return this.prev; }
ms.halfEdgeNet.prototype.getNext    = function() { return this.next; }
ms.halfEdgeNet.prototype.getFace    = function() { return this.face; }
ms.halfEdgeNet.prototype.getNetwork = function() { return this.network; }
ms.halfEdgeNet.prototype.getVertexIndex = function() { return this.vertexIndex; }
ms.halfEdgeNet.prototype.getEdgeIndex = function() { return this.edgeIndex; }

ms.halfEdgeNet.prototype.connectNet = function(network) {
	this.network = network;
	network.addHalfEdge(this);
	return this;
};

ms.halfEdgeNet.prototype.getFaceDatum = function() {
	return this.edge && this.edge.getPrimal().getType().getFaceData()[this.edgeIndex];
};

ms.halfEdgeNet.prototype.getDir = function() {
	if (!this.edge) {
		return null;
	}
	var dir = this.edge.getPrimal().getType().getDir().copy();
	if (!this.forward) {
		return dir.scale(-1);
	} else {
		return dir;
	}
};

ms.halfEdgeNet.prototype.import = function(json) {
	this.forward = json.forward;
	this.edgeIndex = json.edgeIndex;
	this.vertexIndex = json.vertexIndex;
	this.vertex = this.network.getVertices()[json.vertex];
	this.edge   = this.network.getEdges()[json.edge];
	this.prev   = this.network.getHalfEdges()[json.prev];
	this.next   = this.network.getHalfEdges()[json.next];
	this.face   = this.network.getFaces()[json.face];
};

ms.halfEdgeNet.prototype.getTwin = function() {
	var halfEdges = this.edge.getHalfEdges();
	if (halfEdges.length != 2) {
		ms.alert('Expect two halfEdges to get a twin.');
	}
	return this.edge.getHalfEdges()[1 - this.edgeIndex][0];
};

ms.halfEdgeNet.prototype.disconnectHalfEdge = function() {
	this.next && this.next.setPrev(null);
	this.next = null;
};

ms.halfEdgeNet.prototype.connectHalfEdge = function(next) {
	this.next = next;
	next.setPrev(this);
};

ms.halfEdgeNet.prototype.setPrev = function(prev) {
	this.prev = prev;
};

ms.halfEdgeNet.prototype.setFace = function(face) {
	this.face = face;
};

ms.halfEdgeNet.prototype.connectVertex = function(vertex, index) {
	if (index == -1) {
		// Set to next available spot in the vertex.
		index = vertex.getHalfEdges().length;
	}
	this.vertex = vertex;
	this.vertexIndex = index;
	this.vertex.setHalfEdge(this, index);
};

ms.halfEdgeNet.prototype.export = function() {
	return {
		forward: this.getForward(),
		edgeIndex: this.getEdgeIndex(),
		vertexIndex: this.getVertexIndex(),
		vertex: this.network.vertexIndex(   this.getVertex()),
		edge:   this.network.edgeIndex(     this.getEdge()),
		prev:   this.network.halfEdgeIndex( this.getPrev()),
		next:   this.network.halfEdgeIndex( this.getNext()),
		face:   this.network.faceIndex(     this.getFace()),
	};
};

ms.halfEdgeNet.prototype.boundaryToInterior = function() {
	var edge = this.getEdge();
	var startHalf = edge && edge.getPrimal().getInterior().getOuterComponent();
	if (this.forward) {
		var endHalf = startHalf;
		var next = endHalf.getNext();
		while (next) {
			endHalf = next;
			next = endHalf.getNext();
		}
		return endHalf;
	} else {
		return startHalf;
	}
};

ms.halfEdgeNet.prototype.isSpliced = function() {
	return this.getEdge() && this.getEdge().getPrimal().getType().spliced;
};

// If is part of a loop rather than ending in a connector.
ms.halfEdgeNet.prototype.isLoopy = function() {
	var connected = ms.faceNet.getConnectedHalfEdges(this);
	var last = connected.pop();
	return !!last.getNext();
};
	
ms.halfEdgeNet.prototype.print = function() {
	ms.highlight(this.getHalfEdgeSet());
	return this.id;
};

ms.halfEdgeNet.prototype.getHalfEdgeSet = function(opt_drawBackwards) {
	var interior = this.edge && this.edge.primal.getInterior();
	if (this.edge && (interior != this.edge)) {
		var result = interior.getHalfEdgeSet();
		if (!this.forward) {
			result.reverse();
		}
		return result;
	} else {
		return new ms.halfEdgeSet([this]);
	}
};



ms.halfEdgeSet = function(halfEdges, isBackwards) {
	this.halfEdges = halfEdges;
	this.isBackwards = isBackwards;
};

ms.halfEdgeSet.create = function(halfEdges, isBackwards) {
	return {halfEdgeSet: new ms.halfEdgeSet(halfEdges, isBackwards)};
};

ms.halfEdgeSet.prototype.reverse = function() {
	this.isBackwards = new Array(this.halfEdges.length).fill(true);
};

ms.halfEdgeSet.prototype.concat = function(setB) {
	this.halfEdges = this.halfEdges.concat(setB.halfEdges);
	this.isBackwards = this.isBackwards.concat(setB.isBackwards);
};

ms.halfEdgeSet.prototype.highlight = function(view) {
	var options = {halfEdgeSet: this};
	this.halfEdges[0].network.highlight(view, options);
};

ms.halfEdgeSet.prototype.requiresShapeView = function () {
	return true;
};

