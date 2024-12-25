ms.ringInstance = function(ring, stats) {
	this.ring = ring;
	var numEdges = ring.getGraph().getEdges().length;
	var properties = {
		line: new ms.requiredArray('line', /* required */ false, numEdges ),
	};
	this.node = new ms.node(this, stats, 'ringInstance', properties);
};

ms.ringInstance.maxSegmentLength = 2;

ms.ringInstance.prototype.getNode = function() {
	return this.node;
};

ms.ringInstance.prototype.getLines = function() {
	return this.node.get('line');
};

ms.ringInstance.prototype.getRing = function() {
	return this.ring;
};

ms.ringInstance.prototype.copy = function() {
	var copy = new ms.ringInstance(this.ring, this.node.getStats());
	var lines = this.getLines();
	for (var i = 0; i < lines.length; i++) {
		copy.setLine(lines[i], i);
	}
	return copy;
};

ms.ringInstance.prototype.setLine = function(line, index) {
	this.node.insert(line, index);
};

ms.ringInstance.prototype.randomTransitionGroups = function(edgeTypeStarted) {
	var spot = ms.pick(this.ring.getTransitionSpots());
	var startIndex = spot.groupIndex;
	var groups = spot.transition.getGroups();
	var probabilities = new Array(groups.length).fill(1);
	probabilities[startIndex] = 0;

	// Do not allow a new fully connected edge to be created if it already has been.
	if (groups[0].isEmpty()) {
		for (var i = 0; i < groups.length; i++) {
			var connectedTypes = groups[i].getFullyConnectedTypes();
			var started = connectedTypes.some(function(id) {
				return edgeTypeStarted.includes(id);
			});
			if (started) {
				probabilities[i] = 0;
			}
		}
	}
	
	var endIndex = ms.randomDistribution(probabilities);
	// This can happen when the only starter rule is the boundary rule.
	if (endIndex < 0) {
		return null;
	}

	var startInstance = new ms.ringGroupInstance(groups[startIndex]);
	startInstance.addRingInstance(spot.ringIndex, this);
	return {startInstance: startInstance, endGroup: groups[endIndex]};
};

// TODO: We might be able to just look at the outer endpoints and that might be faster.
ms.ringInstance.prototype.hasCommonVertices = function(instanceB) {
	var linesA = this.getLines();
	var verticesA = [];
	linesA.forEach(function(line) {
		var endpoints = line.getEndpoints();
		verticesA.push(endpoints[0].getVertex());
		verticesA.push(endpoints[1].getVertex());
	});
	var linesB = instanceB.getLines();
	for (var i = 0; i < linesB.length; i++) {
		var endpointsB = linesB[i].getEndpoints();
		if (verticesA.includes(endpointsB[0].getVertex()) || verticesA.includes(endpointsB[1].getVertex())) {
			return true;
		}
	}
	return false;
};

ms.ringInstance.prototype.isFull = function() {
	var entries = this.getLines();
	return !entries.some(function(entry) { return !entry; });
};

ms.ringInstance.prototype.highlight = function(context, convertToScreen) {
	if (this.node.isDestroyed()) {
		return;
	}
	for (var i = 0; i < this.getLines().length; i++) {
		var line = this.getLines()[i];
		line && line.highlight(context, convertToScreen);
	}
};

ms.ringInstance.prototype.print = function() {
	ms.highlight(this);
};
