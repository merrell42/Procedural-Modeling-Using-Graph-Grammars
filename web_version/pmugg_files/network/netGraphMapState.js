ms.netGraphMapState = function(info, opt_map) {
	this.info = info;
	this.map = opt_map || ms.netGraphMap.create(info);
	// A queue of the half-edges to be checked.
	this.queue = [];
	// A queue of the half-edges to splice.
	this.spliceQueue = [];
};

ms.netGraphMapState.prototype.setSetB = function(setB) {
	this.setB = setB;
};

ms.netGraphMapState.prototype.setQueue = function(queue) {
	this.queue = queue;
};

ms.netGraphMapState.prototype.graphA = function() {
	return this.info.graphA;
};

ms.netGraphMapState.prototype.graphB = function() {
	return this.info.graphB;
};

ms.netGraphMapState.prototype.copy = function() {
	var result = new ms.netGraphMapState(this.info.copy(), this.map.copy());
	var queue = this.queue.slice();
	result.setQueue(queue);
	return result;
};

ms.netGraphMapState.prototype.assignVertex = function(vertexA, indexB) {
	var info = this.info;
	this.map.vertexBtoA[indexB] = vertexA;

	var vertexB = info.verticesB[indexB];
	var halfsB = vertexB.getHalfEdges();
	for (var i = 0; i < halfsB.length; i++) {
		if (halfsB[i].isSpliced()) {
			this.spliceQueue.push({halfB: halfsB[i], vertexA});
		} else {
			this.queue.push({halfB: halfsB[i], vertexA});
		}
	}
};

ms.netGraphMapState.prototype.assignHalf = function(indexA, indexB) {
	this.map.halfAtoB[indexA] = indexB;
	this.map.halfBtoA[indexB] = indexA;
};

ms.netGraphMapInfo = function(nodeStats, netB) {
	this.nodeStats = nodeStats;
	this.netB = netB;

	// this.verticesA = netA.getConnectors();
	this.verticesB = netB.getInterior().getVertices();
	this.edgesB    = netB.getInterior().getEdges();

	// The graph index for graph A and B within the set.
	/* this.indexA = indexA;
	this.indexB = indexB;
	this.getVertexSignature = getVertexSignature;
	this.getEdgeSignature = getEdgeSignature;
	this.numEdgesA = graphA.getEdges().length;
	this.numEdgesB = graphB.getEdges().length; */
};

ms.netGraphMapInfo.prototype.copy = function() {
	return new ms.netGraphMapInfo(this.netA, this.netB);
};

// For the vertex at the given index in graphA get its signature.
ms.netGraphMapInfo.prototype.vSignA = function(index) {
	return this.netA.getConnectors()[index].signature();
};

// For the vertex at the given index in graphB get its signature.
ms.netGraphMapInfo.prototype.vSignB = function(index) {
	return this.netB.getConnectors()[index].signature();
};

// For the edge at the given index in graphA get its signature.
ms.netGraphMapInfo.prototype.eSignA = function(index) {
	return this.getEdgeSignature(this.graphA.getEdges()[index])
};

// For the edge at the given index in graphA get its signature.
ms.netGraphMapInfo.prototype.eSignB = function(index) {
	return this.getEdgeSignature(this.graphB.getEdges()[index])
};