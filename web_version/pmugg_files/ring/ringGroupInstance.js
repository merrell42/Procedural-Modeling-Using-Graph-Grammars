ms.ringGroupInstance = function(group) {
	this.group = group;
	var numRings = this.group.getRings().length;
	this.ringInstances = new Array(numRings).fill(null);
};

ms.ringGroupInstance.prototype.getStats = function() {
	return this.ringInstances[0].getNode().getStats();
};

ms.ringGroupInstance.prototype.addRingInstance = function(index, instance) {
	this.ringInstances[index] = instance;
};

// Check if the new instance is compatible with the existing ones.
ms.ringGroupInstance.prototype.isCompatible = function(instanceB) {
	return !this.ringInstances.some(function(instanceA) {
		return instanceA && instanceA.hasCommonVertices(instanceB);
	});
};

ms.ringGroupInstance.prototype.getGroup = function() {
	return this.group;
};

ms.ringGroupInstance.prototype.getLines = function() {
	var lines = [];
	for (var i = 0; i < this.ringInstances.length; i++) {
		var linesI;
		if (this.ringInstances[i]) {
			linesI = this.ringInstances[i].getLines();
		}	else {
		  linesI = new Array(this.group.getRings()[i].getGraph().getEdges().length);
			linesI.fill(null);
		}
		lines = lines.concat(linesI);
	}
	return lines;
};

ms.ringGroupInstance.prototype.destroy = function() {
	this.ringInstances.forEach(function(instance) {
		instance && instance.getNode().destroy();
	});
};

ms.ringGroupInstance.findRingAttempts = 5;

ms.ringGroupInstance.prototype.findMissingRings = function(newGroup, mutationArea, ringTree) {
	for (var i = 0; i < ms.ringGroupInstance.findRingAttempts; i++) {
		var found = this.findMissingRingsOneAttempt(newGroup, mutationArea, ringTree);
		if (found) {
			return true;
		}
	}
	return false;
};

ms.ringGroupInstance.prototype.findMissingRingsOneAttempt = function(newGroup, mutationArea, ringTree) {	
	var searchForward = (ms.random(2) == 0);
	var possibleTransitions = [];

	// A possible transition is an existing instance and a missing instance next to each other.
	var n = this.ringInstances.length;
	var nextIndex = function(i) {
		return searchForward ? (i + 1) % n : (i - 1 + n) % n
	}
	for (var i = 0; i < n; i++) {
		if (this.ringInstances[i] && !this.ringInstances[nextIndex(i)]) {
			possibleTransitions.push(i);
		}
	}
	if (possibleTransitions.length == 0) {
		return true;
	}
	var existingIndex = ms.pick(possibleTransitions);
	var missingIndex = nextIndex(existingIndex);
	var existingLines = this.ringInstances[existingIndex].getLines();
	
	var newRing = ms.pickOne(newGroup.getRings());
	var path = [];
	var endpointIndex = searchForward ? this.group.findLast(existingIndex) : this.group.findFirst(existingIndex);
	var endpoint = newRing.getOuterEndpoints()[endpointIndex];
	var increment = function() {
		path.push(endpoint);
		if (searchForward) {
			endpoint = endpoint.next();
		} else {
			endpoint = endpoint.twin().counter();
		}
	};
	increment();
	while (!endpoint.getVertex().isOuter()) {
		increment();
	}
	var line = this.getLines()[this.group.endpointToLineIndex(endpointIndex)];
	var isAtStart = this.group.getEndpoint(endpointIndex).getIsAtStart();
	var coreEndpoint = line.getEndpoints()[isAtStart ? 0 : 1];
	
	var intersection = ms.ringGroupInstance.constructPath(coreEndpoint, path, existingLines, mutationArea);
	if (intersection) {
		var missingId = this.group.getRings()[missingIndex].getId();
		var line = intersection.state.getLine();
		var ringInstances = ringTree[missingId].getRingInstances(line);
		if (ringInstances.length == 0) {
			return false;
		}
		var instance = ms.pick(ringInstances);
		if (instance.isFull() && !this.hasCommonLine(instance)) {
			if (instance.getRing().getId() != missingId) {
				ms.alert('Error in the ringSlots computation.');
			}
			if(!this.isCompatible(instance)) {
				return false;
			}
			this.addRingInstance(missingIndex, instance);
			return this.findMissingRings(newGroup, mutationArea, ringTree);
		}			
	}
	return false;
};

ms.ringGroupInstance.randomAngle = 50 / 180 * Math.PI;

ms.ringGroupInstance.constructPath = function(coreEndpoint, path, existingLines, mutationArea) {
	var pathLines = [];
	var cleanup = function() {
		pathLines.forEach(function(line) {
			line.getNode().destroy();
		});
	};
	
	var isValidIntersection = function(state) {
		return !(existingLines.includes(state.getLine()));
	};

	if (path.length > 1) {
		path.pop();
	}
	var d = ms.ringInstance.maxSegmentLength;
	var position0 = coreEndpoint.getVertex().getPosition();
	var length0 = position0.distance(coreEndpoint.getTwin().getVertex().getPosition());
	var randomOffset = ms.randomUniform(-ms.ringGroupInstance.randomAngle, ms.ringGroupInstance.randomAngle);
	var angleOffset = coreEndpoint.angleOffset() + randomOffset;
	for (var i = 0; i < path.length; i++) {
		var length;
		if (i == path.length - 1) {
			length = 10;
		} else {
			var minLength = 0;
			var maxLength = d;
			if (i == 0) {
				minLength = Math.max(0, length0 - d / 2);
				maxLength = length0 + d;
			}
			var r = Math.random();
			length = (1 - r) * minLength + r * maxLength;
		}
		var angle = angleOffset + path[i].getAngle();
		var v = ms.vec2.unitVec(angle)
		if (v.x < 0) {
			length = Math.min(length, (mutationArea.lowerExtent[0] - position0.x) / v.x);
		} else if (v.x > 0) {
			length = Math.min(length, (mutationArea.upperExtent[0] - position0.x) / v.x);
		}
		if (v.y < 0) {
			length = Math.min(length, (mutationArea.lowerExtent[1] - position0.y) / v.y);
		} else if (v.y > 0) {
			length = Math.min(length, (mutationArea.upperExtent[1] - position0.y) / v.y);
		}

		position1 = position0.copy().add(v.scale(length));
		
		var nodeStats = coreEndpoint.getNode().getStats();
		var line = ms.line.createFromPositions([position0, position1], angle, path[i].getEdge().getCore(), nodeStats);
		pathLines.push(line);
		var lineState = line.getSegment().getStates()[0];
		var intersection = lineState.addToModelWithIntersections(1e-4, true, false, isValidIntersection);
		if (intersection && intersection.state) {
			cleanup();
			if (intersection.state.getLine()) {
				return intersection;
			} else {
				// If the line is missing it was destroyed in the cleanup which
				// means this path intersect itself and is invalid.
				return null;
			}
		}
		position0 = position1;
	}
	cleanup();
	return null;
};

ms.ringGroupInstance.prototype.hasCommonLine = function(instanceB) {
	var linesB = instanceB.getLines();
	for (var i = 0; i < this.ringInstances.length; i++) {
		if (this.ringInstances[i]) {
			var linesA = this.ringInstances[i].getLines();
			for (var j = 0; j < linesA.length; j++) {
				if (linesB.includes(linesA[j])) {
					return true;
				}
			}
		}
	}
	return false;
};

ms.ringGroupInstance.prototype.mergeEndpoints = function(lineIndices, endpointIndices) {
	this.group = this.group.mergeEndpoints(endpointIndices);
	var lines = this.getLines();
	
	var line0 = lines[lineIndices[0]];
	var self = this;
	this.ringInstances.forEach(function(rInstance, index) {
		var lines = rInstance.getLines();
		if (lines.includes(line0)) {
			lines = lines.filter(function(line) { return (line != line0)});
			lines.push(line0);
			var newInstance = new ms.ringInstance(rInstance.ring, rInstance.node.getStats());
			// HACK: Because the ring doesn't have the right number of lines.
			var oldLines = newInstance.getLines();
			while (oldLines.length > 0) { oldLines.pop(); };
			for (var i = 0; i < lines.length; i++) {
				newInstance.setLine(lines[i], i);
			}
			self.ringInstances[index] = newInstance;
		}
	});

	// Change the order of the edges in the graph to match this.getLines.
	// line0 should go from the end to where it is in getLines.
	var index0 = this.getLines().indexOf(line0);
	var newOrder = [];
	var graph = this.group.getGraph();
	n = graph.getEdges().length;
	for (var i = 0; i < n; i++) {
		newOrder.push(i);
	}
	newOrder.splice(index0, 1);
	newOrder.push(index0);
	graph.edges = ms.familyTreeEdge.reorder(graph.edges, newOrder);

	var endpointData = this.group.getEndpointData();
	endpointData.forEach(function(datum) {
		datum.lineIndex = newOrder[datum.lineIndex];
	});
	return this;
};

ms.ringGroupInstance.prototype.highlight = function(context, convertToScreen) {
	for (var i = 0; i < this.ringInstances.length; i++) {
		this.ringInstances[i] && this.ringInstances[i].highlight(context, convertToScreen);
	}
};

ms.ringGroupInstance.prototype.print = function() {
	ms.highlight(this);
};