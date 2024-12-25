ms.ring = function(angles, windingAngles, endpoints, graph) {
	this.angles = angles;
	this.windingAngles = windingAngles;
	this.graph = graph;
	this.transitionSpots = [];
	this.outerEndpoints = endpoints.slice();
	this.node = null;
	this.cachedHasSingleFragment = null;
	this.id = ms.ring.count++;

	this.endpointIds = endpoints.map(function(endpoint) {
		return endpoint.idOld();
	});
};

ms.ring.prototype.findRings = function(rings) {
	this.transitionSpots.forEach((spot) => spot.transition.findRings(rings));
};

ms.ring.prototype.export = function(types) {
	var graphEndpoints = this.graph.getOuterEndpoints();
	var outerEndpoints = this.outerEndpoints.map((endpoint) => graphEndpoints.indexOf(endpoint));
	return {
		transitionSpots: this.transitionSpots.map((spot) => ({
			transition: spot.transition.export(types),
			groupIndex: spot.groupIndex,
			ringIndex: spot.ringIndex,
		})),
		id: this.id,
		graph: this.graph.export(types),
		outerEndpoints,
	};
};

ms.ring.import = function(json, types) {
	var graph = ms.graph.import(json.graph, types);
	var graphEndpoints = graph.getOuterEndpoints();
	var endpoints = json.outerEndpoints.map((e) => graphEndpoints[e]);
	var result = new ms.ring([], [], endpoints, graph);
	result.id = json.id;
	return result;
};

// Import the transition spots separately to avoid a circular dependency.
ms.ring.prototype.importSpots = function(transitionSpots, types) {
	this.transitionSpots = transitionSpots.map((spot) => ({
		transition: ms.ringTransition.import(spot.transition, types),
		groupIndex: spot.groupIndex,
		ringIndex: spot.ringIndex,
	}));
};

ms.ring.count = 0;

ms.ring.create = function(graph) {
	var endpointWindings = graph.endpointWindings();	
	if (endpointWindings.length > 0) {
		var angles        = endpointWindings[0].map(function(winding) { return winding.endpoint.getAngle(); });
		var windingAngles = endpointWindings[0].map(function(winding) { return winding.winding });
		var endpoints     = endpointWindings[0].map(function(winding) { return winding.endpoint });
		return new ms.ring(angles, windingAngles, endpoints, graph);
	} else {
		return new ms.ring([], [], [], graph);		
	}
};

ms.ring.prototype.copy = function() {
	var result = ms.ring.create(this.graph);
	var node = this.getNode();
	node && result.setNode(node);
	return result;
};

ms.ring.prototype.setNode = function(node) {
	this.node = node;
	node.setRing(this);
};

ms.ring.prototype.getNode = function() {
	return this.node;
};

ms.ring.prototype.getAngles = function() {
	return this.angles;
};

ms.ring.prototype.getEndpointIds = function() {
	return this.endpointIds;
};

ms.ring.prototype.getWindingAngles = function() {
	return this.windingAngles;
};

ms.ring.prototype.getGraph = function() {
	return this.graph;
};

// These represent the outer endpoints of the graph, but they are in a consistent order. 
ms.ring.prototype.getOuterEndpoints = function() {
	return this.outerEndpoints;
};

ms.ring.prototype.getId = function() {
	return this.id;
};

ms.ring.prototype.getTransitionSpots = function() {
	return this.transitionSpots;
};

ms.ring.prototype.addTransitionSpot = function(transitionSpot) {
	this.transitionSpots.push(transitionSpot);
};

ms.ring.prototype.removeTransition = function(transition) {
	this.transitionSpots = this.transitionSpots.filter(function(spot) {
		return spot.transition != transition;
	});
};

ms.ring.prototype.anglesMatch = function(anglesB) {
	var anglesA = this.angles;
	if (anglesA.length != anglesB.length) {
		return false;
	}
	for (var i = 0; i < anglesA.length; i++) {
		if (Math.abs(anglesA[i] - anglesB[i]) > 0.01) {
			return false;
		}
	}
	return true;
};

ms.ring.angleState = {
	A: 0,
	B: 1, 
	EITHER: 2,
	START: 3
}

ms.ring.prototype.isCompatible = function(ringB) {
	var anglesA = this.angles;
	var anglesB = ringB.getAngles();
	var transitions = 0;

	var indexA = 0;
	var indexB = 0;
	var angleState = ms.ring.angleState;
	var state = angleState.START;
	while (indexA < anglesA.length || indexB < anglesB.length) {
		var angleA = (indexA < anglesA.length) ? anglesA[indexA] : -Infinity;
		var angleB = (indexB < anglesB.length) ? anglesB[indexB] : -Infinity;
		var nextState;
		if (angleA > angleB) {
			nextState = angleState.A;
			indexA++;
		} else if (angleA < angleB) {
			nextState = angleState.B;
			indexB++;
		} else {
			nextState = angleState.EITHER;
			indexA++;
			indexB++;
		}
		if (state == angleState.A) {
		  if (nextState == angleState.B) {
				transitions++;
			} else if (nextState == angleState.EITHER) {
				// Act as if A was before B to minimize transitions.
				transitions++;
				nextState = angleState.B;
			}
		} else if (state == angleState.B) {
			if (nextState == angleState.A) {
				transitions++;
			} else if (nextState == angleState.EITHER) {
				// Act as if B was before A to minimize transitions.
				transitions++;
				nextState = angleState.A;
			}
		} else if (state == angleState.EITHER) {
			transitions++;
			if (nextState == angleState.EITHER) {
				// Pick one it shouldn't matter.
				nextState = angleState.A;
				transitions++;
			}
		}
		state = nextState;
	}
	return transitions <= 2;
};

ms.ring.prototype.isEmpty = function() {
	return this.getNode() == null && this.getAngles().length == 0 && this.getEndpointIds().length == 0;
};

ms.ring.prototype.ignoredIfWindingDisabled = function() {
	// I think node is only empty when we have used stubs or maybe matched backwards.
	var node = this.getNode();
	return !this.isEmpty() && node && node.ignoredIfWindingDisabled;
};

ms.ring.prototype.isDescendant = function(ringB) {
	return this.node && this.node.isDescendant(ringB.getNode());
};

ms.ring.prototype.draw = function(view, options) {	
	this.graph.draw(view, options);
};

ms.ring.prototype.hasSingleFragment = function() {
	if (!this.cachedHasSingleFragment) {
		var hasSingleFragment = this.graph.getEdges().some(function(edge) {
			return edge.getCore().singleFragment();
		});
		this.cachedHasSingleFragment = { cache: hasSingleFragment };
	}
	return this.cachedHasSingleFragment.cache;
};

ms.ring.prototype.singleFragmentCount = function() {
	return this.hasSingleFragment() ? 1 : 0;
};

ms.ring.prototype.print = function() {
	this.graph.print();
};
