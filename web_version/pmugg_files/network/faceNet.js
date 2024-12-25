ms.faceNet = function() {
	// I'm not sure that the distinction between the inner and outer components
	// is important. Maybe they can be combined.
	this.outerComponent = null;
	this.innerComponents = [];
	this.primal = null;
	this.network = null;

	this.id = ms.counter.add('faceNet');
};

ms.counter.register('faceNet');

ms.faceNet.prototype.getOuterComponent  = function() { return this.outerComponent; };
ms.faceNet.prototype.getInnerComponents = function() { return this.innerComponents; };
ms.faceNet.prototype.getPrimal          = function() { return this.primal; };
ms.faceNet.prototype.getNetwork         = function() { return this.network; };

ms.faceNet.prototype.setPrimal = function(primal) {
	this.primal = primal;
};

ms.faceNet.prototype.connectNet = function(network) {
	this.network = network;
	network.addFace(this);
	return this;
};

ms.faceNet.prototype.connectOuter = function(halfEdges) {
	this.outerComponent = halfEdges[0];
	var self = this;
	halfEdges.forEach((halfEdge) => {
		halfEdge.setFace(self);
	});
};

ms.faceNet.prototype.makeInner = function(halfEdge) {
	this.innerComponents.push(halfEdge);
	this.outerComponent = null;
};

ms.faceNet.prototype.copyConnection = function(copy) {
	var copyNet = copy.getNetwork();
	this.outerComponent = this.network.convertHalfEdge(copyNet, copy.getOuterComponent());
	var self = this;
	this.innerComponents = copy.getInnerComponents().map((halfEdge) => {
		return self.network.convertHalfEdge(copyNet, halfEdge);
	});
};

ms.faceNet.prototype.export = function() {
	var self = this;
	return {
		outerComponent: this.network.halfEdgeIndex(this.getOuterComponent()),
		// TODO: Fix inner components.
		innerComponents: this.getInnerComponents().map((halfEdge) => {
			// return self.network.halfEdgeIndex(halfEdge);
			// This is super hacky, but right now we only use the inner components for a direction.
			return halfEdge.getDir().export();
		}),
	}
};

ms.faceNet.prototype.import = function(json) {
	this.outerComponent = this.network.getHalfEdges()[json.outerComponent];
	this.innerComponents = json.innerComponents.map((halfEdge) => {
		// return self.network.getHalfEdges()[halfEdge];
		var dir = ms.vec3.import(halfEdge);
		return { getDir: () => dir };
	});
};

ms.faceNet.getConnectedHalfEdges = function(start) {
	var result = [start];
	var current = start.getNext();
	while (current && current != start) {
		result.push(current);
		current = current.getNext();
	}
	return result;
};

ms.faceNet.prototype.getOuterHalfEdges = function() {
	if (this.outerComponent) {
		return ms.faceNet.getConnectedHalfEdges(this.outerComponent);
	} else {
		return [];
	}
};

ms.faceNet.prototype.getInnerHalfEdges = function() {
	var result = [];
	this.innerComponents.forEach((component) => {
		result = result.concat(ms.faceNet.getConnectedHalfEdges(component));
	});
	return result;
};

ms.faceNet.prototype.getHalfEdges = function() {
	return this.getOuterHalfEdges().concat(this.getInnerHalfEdges());
};

ms.faceNet.prototype.highlight = function(view) {
	var options = {halfEdges: this.getHalfEdges(), drawBackwards: this.tempDrawBacks || false};
	this.network.highlight(view, options);
};

ms.faceNet.prototype.getHalfEdgeSet = function() {
	var halfEdges = this.getHalfEdges();
	if (!this.primal.interior) {
		var result = new ms.halfEdgeSet([], []);
		halfEdges.forEach((halfEdge) => {
			result.concat(halfEdge.getHalfEdgeSet());
		});
		return result;
	} else {
		return new ms.halfEdgeSet(halfEdges);
	}
};

ms.faceNet.prototype.merge = function(faceB) {
	var bInner = faceB.getInnerComponents();
	this.innerComponents = this.innerComponents.concat(bInner);
	var halfEdges = ms.faceNet.getConnectedHalfEdges(faceB.getOuterComponent());
	for (var i = 0; i < bInner.length; i++) {
		halfEdges = halfEdges.concat(ms.faceNet.getConnectedHalfEdges(bInner[i]));
	}
	var self = this;
	halfEdges.forEach((halfEdge) => {
		halfEdge.setFace(self);
	});
	ms.boundNet.mergeInto(this, faceB);
};

// Replace A with B. Or if force is true, for B to be the new value.
// force is needed for loop gluing in the boundary network, but somehow this
// messes up the connectors, so it's disabled for interior networks.
ms.faceNet.prototype.replaceHalfEdge = function(a, b, force) {
	if ((this.outerComponent == a) || (force && this.outerComponent)) {
		this.outerComponent = b;
	}
	for (var i = 0; i < this.innerComponents.length; i++) {
		if ((this.innerComponents[i] == a) || (force && this.innerComponents[i]))  {
			this.innerComponents[i] = b;
		}
	}
	b.setFace(this);
};

ms.faceNet.prototype.isLoopy = function() {
	return this.outerComponent.isLoopy();
};

// This is just a sanity check. This should be true if this is valid.
ms.faceNet.prototype.inNetwork = function() {
	return this.network.getFaces().includes(this);
};

ms.faceNet.prototype.print = function(opt_drawBackwards) {
	ms.highlight(this.getHalfEdgeSet(opt_drawBackwards));
};
