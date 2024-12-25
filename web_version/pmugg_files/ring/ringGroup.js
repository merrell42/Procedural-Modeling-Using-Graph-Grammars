ms.ringGroup = function(endpointIds, endpointWindings) {
	this.rings = [];
	this.endpointData = [];
	for (var i = 0; i < endpointIds.length; i++) {
		this.endpointData[i] = {endpoint: null, id: endpointIds[i], winding: endpointWindings[i], ringIndex: -1, lineIndex: -1, endpointIndex: -1};
	}
	this.numFilledEndpoints = 0;
	this.graph = new ms.graph();
	this.outerEndpoints = null;
	this.fullyConnectedTypes = null;
	this.id = ms.ringGroup.count++;
};

ms.ringGroup.count = 0;

ms.ringGroup.prototype.findRings = function(rings) {
	ms.union(rings, this.rings);
};

ms.ringGroup.prototype.export = function(types) {
	var self = this;
	endpointData = this.endpointData.map((datum) => {
		var result = {...datum};
		delete result.endpoint;
		result.eIndex = self.rings[datum.ringIndex].getOuterEndpoints().indexOf(datum.endpoint);		
		return result;
	});
	this.rings.map((ring) => {
		if (types.rings.indexOf(ring) == -1) {
			console.log('Cannot find ring');
		}
	});
	return {
		rings: this.rings.map((ring) => types.rings.indexOf(ring)),
		graph: this.graph.export(types),
		endpointData,
	};
};

ms.ringGroup.import = function(json, types) {
	var result = new ms.ringGroup([], [], null);
	result.rings = json.rings.map((ring) => types.rings[ring]);
	result.graph = ms.graph.import(json.graph, types);
	result.endpointData = json.endpointData.map((datum) => {
		datum.endpoint = result.rings[datum.ringIndex].getOuterEndpoints()[datum.eIndex];
		return datum;
	});
	return result;
};

// The cost of adding another ring. This is used when considering which
// ringGroups to use in the rules.
ms.ringGroup.ringCost = 10;

ms.ringGroup.create = function(endpointIds, endpointWindings) {
	return new ms.ringGroup(endpointIds, endpointWindings);
};

ms.ringGroup.createFromRing = function(ring) {
	var group = ms.ringGroup.create(ring.getEndpointIds(), ring.getWindingAngles());
	group.fillFromRing(ring);
	return group;
};

ms.ringGroup.createFromRings = function(rings) {
	var endpointIds = [];
	var windingAngles = [];
	var indices = [];
	var index = 0;
	rings.forEach(function(ring) {
		endpointIds = endpointIds.concat(ring.getEndpointIds());
		windingAngles = windingAngles.concat(ring.getWindingAngles());
		var ringIndices = [];
		for (var i = 0; i < ring.getEndpointIds().length; i++) {
			ringIndices.push(index);
			index++;
		}
		indices.push(ringIndices);
	});
	var group = ms.ringGroup.create(endpointIds, windingAngles);
	rings.forEach(function(ring, rIndex) {
		var holeData = {indices: indices[rIndex]};
		group.fillHole(ms.ringGroup.createFromRing(ring));
	});
	return group;
};

ms.ringGroup.prototype.getCost = function() {
	var cost = 0;
	for (var i = 0; i < this.rings.length; i++) {
		cost += this.rings[i].getNode().getCost();
	}
	return cost + ms.ringGroup.ringCost * this.rings.length;
};

ms.ringGroup.prototype.copy = function() {
	var copyB = ms.ringGroup.create([], []);
	for (var i = 0; i < this.endpointData.length; i++) {
		var datum = this.endpointData[i];
		copyB.endpointData.push({
			id: datum.id,
			ringIndex: datum.ringIndex,
			lineIndex: datum.lineIndex,
			endpoint: datum.endpoint,
			winding: datum.winding,
			endpointIndex: datum.endpointIndex,
		});
	}
	copyB.rings = this.rings.slice();
	copyB.graph = this.graph.copy();
	copyB.numFilledEndpoints = this.numFilledEndpoints;
	return copyB;
};

ms.ringGroup.prototype.getRings = function() {
	return this.rings;
};

ms.ringGroup.prototype.getOuterEndpoints = function() {
	if (!this.outerEndpoints) {
		this.outerEndpoints = this.graph.getOuterEndpoints();
		if (this.outerEndpoints.length != this.endpointData.length) {
			ms.alert('Wrong length of the outer endpoints.');
		}
	}
	return this.outerEndpoints;
};

ms.ringGroup.prototype.getGraph = function() {
	return this.graph;
};

ms.ringGroup.prototype.numMissingEndpoints = function() {
	return this.endpointData.length - this.numFilledEndpoints;
};

ms.ringGroup.prototype.getFullyConnectedTypes = function() {
	if (this.fullyConnectedTypes == null) {
		var connectedTypes = [];
		this.rings.forEach(function(ring) {
			ring.getGraph().edges.forEach(function(edge) {
				if (edge.core.isConnected()) {
					var id = edge.core.id;
					if (!connectedTypes.includes(id)) {
						connectedTypes.push(id);
					}
				}
			});
		});
		this.fullyConnectedTypes = connectedTypes;
	}
	return this.fullyConnectedTypes;
};

ms.ringGroup.prototype.addRing = function(ringA, opt_no_reorder) {
	var graphA = ringA.getGraph();
	var newRingIndex = this.rings.length;
	this.rings.push(ringA);
	this.graph.merge(graphA);

	if (opt_no_reorder !== true) {
		var prevOrdering = [];
		var nextOrdering = [];
		var validIndices = [];
		for (var i = 0; i < this.endpointData.length; i++) {
			var datum = this.endpointData[i];
			if (datum.endpoint !== null && datum.ringIndex <= newRingIndex) {
				validIndices.push(i);
				if (datum.ringIndex < newRingIndex) {
					prevOrdering.push(i);
				} else {
					nextOrdering[datum.endpointIndex] = i;
				}
			}
		}
		nextOrdering = nextOrdering.map(function(x) { return validIndices.indexOf(x); });
		prevOrdering = prevOrdering.map(function(x) { return validIndices.indexOf(x); });
		var newOrdering = prevOrdering.concat(nextOrdering);		
		this.getGraph().getOuterVertex().reorderEndpoints(newOrdering);
	}
};

ms.ringGroup.prototype.endpointToLineIndex = function(endpointIndex) {
	return this.endpointData[endpointIndex].lineIndex;
};

ms.ringGroup.prototype.getEndpoint = function(endpointIndex) {
	return this.endpointData[endpointIndex].endpoint;
};

ms.ringGroup.prototype.getHoleDataAfterIndex = function(startIndex) {
	var holeData = [];
	var endpointIds = [];
	var windings = [];
	
	for (var i = startIndex; i < this.endpointData.length; i++) {
		if (this.endpointData[i].endpoint === null) {
			endpointIds.push(this.endpointData[i].id);
			windings.push(this.endpointData[i].winding);
		}
		if (this.endpointData[i].endpoint !== null && endpointIds.length > 0) {
			return {ids: endpointIds, windings: windings};
		}
	}
	return {ids: endpointIds, windings: windings};
};

ms.ringGroup.prototype.findHoleStart = function() {
	if (this.endpointData[0].endpoint === null) {
		var holeStart = this.endpointData.length - 1;
		while (this.endpointData[holeStart].endpoint === null) {
			holeStart--;
			// If all endpoints are empty, return 0.
			if (holeStart < 0) {
				return 0;
			}
		}
		return holeStart + 1;
	} else {
		var holeStart = 0;
		while (this.endpointData[holeStart].endpoint !== null) {
			holeStart++;
		}
		return holeStart;
	}
};

ms.ringGroup.prototype.getHoleData = function() {
	if (this.endpointData[0].endpoint === null) {
		var holeStart = this.findHoleStart();
		var startData = this.getHoleDataAfterIndex(0);
		var endData = this.getHoleDataAfterIndex(holeStart);
		
		return {
			ids: startData.ids.concat(endData.ids),
			windings: startData.windings.concat(endData.windings)
		}
	} else {
		return this.getHoleDataAfterIndex(0);
	}
};

ms.ringGroup.prototype.fillHole = function(holeFiller) {
	var lineCount = this.graph.getEdges().length;
	var ringCount = this.rings.length;
	var holeStart = this.findHoleStart();
	// This is the part I'm changing. I don't think it affects anything.
	// var fillerEndpoints = holeFiller.getOuterEndpoints();
	var fillerEndpoints = holeFiller.endpointData.map(function(datum) { return datum.endpoint; });
	var groupLength = this.endpointData.length;

	// Reorganize the indices. As they were put in getHoleData.
	var startIndices = [];
	var endIndices = [];
	for (var i = 0; i < fillerEndpoints.length; i++) {
		if (i + holeStart < groupLength) {
			endIndices.push(i + holeStart);
		} else {
			startIndices.push(i + holeStart - groupLength);
		}
	}
	var groupIndices = startIndices.concat(endIndices);
	
	for (var i = 0; i < fillerEndpoints.length; i++) {
		if (this.endpointData[groupIndices[i]].id != fillerEndpoints[i].idOld()) {
			ms.alert('ID mismatch');
		}
		var datum = this.endpointData[groupIndices[i]];
		var fillerDatum = holeFiller.getEndpointData()[i];
		datum.endpoint = fillerEndpoints[i];
		datum.lineIndex = lineCount + fillerDatum.lineIndex;
		datum.ringIndex = ringCount + fillerDatum.ringIndex;
		datum.endpointIndex = fillerDatum.endpointIndex;
	}
	var fillerRings = holeFiller.getRings();
	for (var i = 0; i < fillerRings.length; i++) {
		this.addRing(fillerRings[i]);
	}
};

ms.ringGroup.prototype.getEndpointData = function () {
	return this.endpointData;
};

ms.ringGroup.prototype.nextOuterEndpointData = function () {
	return this.endpointData[this.numFilledEndpoints];
};

ms.ringGroup.prototype.insertEndpoint = function(endpoint, endpointIndex, addIndex) {
	var lineIndex = -1;
	var ringIndex = -1;
	if (endpoint) {
		lineIndex = endpoint.getEdge().getIndex();
		ringIndex = 0;
	}
	this.endpointData[endpointIndex].lineIndex = lineIndex;
	this.endpointData[endpointIndex].ringIndex = ringIndex;
	this.endpointData[endpointIndex].endpoint = endpoint;
	this.endpointData[endpointIndex].endpointIndex = addIndex;
};

ms.ringGroup.prototype.addEndpoint = function(endpoint, addIndex, opt_ringIndex) {
	var endpointIndex = this.numFilledEndpoints;
	this.insertEndpoint(endpoint, endpointIndex, addIndex);
	if (opt_ringIndex !== undefined) {
		this.endpointData[endpointIndex].ringIndex = opt_ringIndex;
	}
	this.numFilledEndpoints++;
};

ms.ringGroup.prototype.fillFromRing = function(ring) {
	var ringIndex = this.rings.length;
	this.addRing(ring, true);
	
	var endpoints = ring.getOuterEndpoints();
	for (var i = 0; i < endpoints.length; i++) {
		this.addEndpoint(endpoints[i], i, ringIndex);
	}
};

ms.ringGroup.prototype.removeTransition = function(transition) {	
	this.getRings().forEach(function(ring) {			
		ring.removeTransition(transition);
	});
};

// Find the first instance of a particular ringIndex. The endpoints wrap around.
ms.ringGroup.prototype.findFirst = function(ringIndex) {
	// Handle the case where the indices wrap around.
	if (this.endpointData[0].ringIndex == ringIndex && this.endpointData[this.endpointData.length - 1].ringIndex == ringIndex) {
		var i = this.endpointData.length - 1;
		while (this.endpointData[i - 1].ringIndex == ringIndex) {
			i--;
		}
		return i;
	}
	var i = 0;
	while (this.endpointData[i].ringIndex != ringIndex) {
		i++;
	}
	return i;
};

// Find the last instance of a particular ringIndex. The endpoints wrap around.
ms.ringGroup.prototype.findLast = function(ringIndex) {
	var data = this.endpointData;
	// Handle the case where the indices wrap around.
	if (this.endpointData[0].ringIndex == ringIndex && this.endpointData[this.endpointData.length - 1].ringIndex == ringIndex) {
		var i = 0;
		while (this.endpointData[i + 1].ringIndex == ringIndex) {
			i++;
		}
		return i;
	}
	var i = this.endpointData.length - 1;
	while (this.endpointData[i].ringIndex != ringIndex) {
		i--;
	}
	return i;
};

ms.ringGroup.prototype.mergeEndpoints = function(indices) {
	indices.sort();
	var result = this.copy();
	var endpointData = result.getEndpointData();
	var ringIndex = endpointData[indices[0]].ringIndex;
	var endpoint0 = endpointData[indices[0]].endpoint;
	var endpoint1 = endpointData[indices[1]].endpoint;
	// The result is degenerate if we are merging a line segment.
	if (endpoint0.getGraph().getEdges().length == 1 ||
	    endpoint1.getGraph().getEdges().length == 1) {
		return null;
	}
	var attached = ms.graphEndpoint.attachEndpoints(endpoint0, endpoint1);
	for (var i = 0; i < result.endpointData.length; i++) {
		if (attached.aDest[i]) {
			result.endpointData[i].endpoint = attached.aDest[i];
			result.endpointData[i].lineIndex = attached.aDest[i].getEdge().getIndex()
		}
	}
	result.rings[ringIndex] = ms.ring.create(attached.newGraph);

	// It's kind of weird that there are two graphs.
	attached = ms.graphEndpoint.attachEndpoints(result.getOuterEndpoints()[indices[0]], result.getOuterEndpoints()[indices[1]]);
	result.graph = attached.newGraph;
	for (var i = 0; i < result.endpointData.length; i++) {
		if (attached.aDest[i]) {
			result.endpointData[i].lineIndex = attached.aDest[i].getEdge().getIndex()
		}
	}
	result.endpointData.splice(indices[1], 1);
	result.endpointData.splice(indices[0], 1);
	result.outerEndpoints = null;
	return result;
};

ms.ringGroup.prototype.requiresShapeView = function () {
	return true;
};

ms.ringGroup.prototype.singleFragmentCount = function() {
	var count = 0;
	for (var i = 0; i < this.rings.length; i++) {
		count += this.rings[i].singleFragmentCount()
	}
	return count;
};

ms.ringGroup.prototype.isEmpty = function() {
	return this.rings.length == 1 && this.rings[0].isEmpty();
};

ms.ringGroup.prototype.highlight = function(view, opt_options) {
	var options = opt_options || {};
	var size = ms.graph.HIGHLIGHTED_SIZE;
	var offset = options.offset || new ms.vec2(20, view.canvas.height - size - 20);
	options.rect = [offset.x, offset.x + size, offset.y, offset.y + size, 0, 100];
	for (var i = 0; i < this.rings.length; i++) {
		this.rings[i].getGraph().draw(view, options);
		options.rect[0] += ms.graph.HIGHLIGHTED_SIZE;
		options.rect[1] += ms.graph.HIGHLIGHTED_SIZE;
	}
};

ms.ringGroup.prototype.ignoredIfWindingDisabled = function() {
	return this.rings.some(function(ring) {
		return ring.ignoredIfWindingDisabled();
	});
};

ms.ringGroup.equalsSet = function(group, set) {
	if (!ms.util.arraysEqual(group.rings, set.rings)) {
		return false;
	}
	var n = group.endpointData.length;
	var connectors = set.getConnectors();
	if (n != connectors.length) {
		return false;
	}
	if (!group.graph.equals(set.graph)) {
		return false;
	}
	for (var i = 0; i < n; i++) {
		var datum = group.endpointData[i];
		var connector = connectors[i];
		var spot = connector.spot;
		if (!datum.endpoint.equals(spot.endpoint)
			// datum.endpoint can have some random extra endpoints.
			/* || !datum.endpoint.getGraph().equals(spot.endpoint.getGraph()) */) {
			return false;
		}
		/* if (datum.endpoint != spot.endpoint) {
			return false;
		} */
		if (datum.ringIndex != spot.ringIndex) {
			return false;
		}
		if (datum.lineIndex != spot.lineIndex) {
			return false;
		}
	}
	return true;
};

ms.ringGroup.prototype.print = function() {
	if (this.rings.length == 1) {
		this.rings[0].print();
	} else {
		ms.highlight(this);
	}
};