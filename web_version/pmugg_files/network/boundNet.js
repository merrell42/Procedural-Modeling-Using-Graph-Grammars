// Two networks representing the interior and boundary of a graph.
//          Vertex Edge Face
// Interior Vertex Edge Face
// Boundary Vertex Face Volume
ms.boundNet = function(interior, boundary, types) {
	this.interior = interior;
	this.boundary = boundary;
	this.interior.setBoundNet(this);
	this.boundary.setBoundNet(this);
	this.cachedConnectors = null;
	this.types = types;

	this.id = ms.counter.add('boundNet');
};

ms.boundNet.prototype.getBoundary = function(){	return this.boundary; };
ms.boundNet.prototype.getInterior = function(){	return this.interior; };
ms.boundNet.prototype.getTypes = function(){	return this.types; };

ms.boundNet.connectBoundary = function(primal, boundary) {
	if (!boundary) {
		return;
	}
	primal.boundary.push(boundary);
	boundary && boundary.setPrimal(primal);
};

ms.boundNet.connectInterior = function(primal, interior) {
	if (!interior) {
		return;
	}
	primal.interior.push(interior);
	interior && interior.setPrimal(primal);	
};

/* ms.boundNet.prototype.copy = function() {		
	var exported = this.export();
	return ms.boundNet.import(exported, this.types);	
} */

ms.boundNet.prototype.copy = function() {
	ms.timerG.start('Copy Net');
	// Copy the two networks.
	var copyI = this.interior.copy();
	var copyB = this.boundary.copy();
	var resultB = new ms.boundNet(copyI, copyB);
	var self = this;
	
	// Create a copy of primalA with the given interior and boundary.
	var createPrimal = function(primalA, interiorB, boundaryB) {
		var primalB = primalA.copy();
		ms.boundNet.connectBoundary(primalB, boundaryB);
		ms.boundNet.connectInterior(primalB, interiorB);
	};

	// Copy the primal vertices, edges, faces, and volumes.
	this.interior.getVertices().forEach((objA) => {
		var primalA = objA.getPrimal();
		var interiorB = resultB.interior.convertVertex(self.interior, primalA.getInterior());
		var boundaryB = resultB.boundary.convertVertex(self.boundary, primalA.getBoundary());
		createPrimal(primalA, interiorB, boundaryB);
	});
	this.interior.getEdges().forEach((objA) => {
		var primalA = objA.getPrimal();
		var interiorB = resultB.interior.convertEdge(self.interior, primalA.getInterior());
		var boundaryB = null;
		createPrimal(primalA, interiorB, boundaryB);
	});
	this.interior.getFaces().forEach((objA) => {
		var primalA = objA.getPrimal();
		var interiorB = resultB.interior.convertFace(self.interior, primalA.getInterior());
		var boundaryB = resultB.boundary.convertEdge(self.boundary, primalA.getBoundary());
		createPrimal(primalA, interiorB, boundaryB);
	});
	this.boundary.getFaces().forEach((objA) => {
		var primalA = objA.getPrimal();
		var interiorB = null;
		var boundaryB = resultB.boundary.convertFace(self.boundary, primalA.getBoundary());
		createPrimal(primalA, interiorB, boundaryB);
	});
	ms.timerG.stop('Copy Net');
	return resultB;
}

ms.boundNet.import = function(json, types) {
	var boundNet = new ms.boundNet(
		ms.network.import(json.interior),
		ms.network.import(json.boundary),
		types
	);
	
	// Create a copy of primalA with the given interior and boundary.
	var createPrimal = function(primalA, interiorB, boundaryB) {
		var primalB = primalA.copy();
		ms.boundNet.connectBoundary(primalB, boundaryB);
		ms.boundNet.connectInterior(primalB, interiorB);
	};

	// Import the primal vertices, edges, faces, and volumes.
	json.vertices.forEach((json) => {
		var vertex = new ms.primalVertex();
		vertex.import(boundNet, types, json);
	});
	json.edges.forEach((json) => {
		var edge = new ms.primalEdge();
		edge.import(boundNet, types, json);
	});
	json.faces.forEach((json) => {
		var face = new ms.primalFace();
		face.import(boundNet, types, json);
	});
	json.volumes.forEach((json) => {
		var volume = new ms.primalVolume();
		volume.import(boundNet, types, json);
	});
	return boundNet;
};

ms.boundNet.prototype.getConnectors = function() {
	if (!this.cachedConnectors) {
		this.cachedConnectors = [];
		var self = this;
		var faces = this.getBoundary().getFaces();
		/* if (faces.length > 1) {
			faces = faces.filter((f) => f.getOuterComponent().getForward());
		} */
		faces.forEach((face) => {
			var outerHalfs = face ? face.getOuterHalfEdges() : [];
			outerHalfs.forEach((half) => {
				if (half.getNext() == half && !half.getVertex()) {
					// This is an outer face with no connectors.
					return;
				}
				var connector = half.boundaryToInterior().getVertex().getPrimal();
				if (!self.cachedConnectors.includes(connector)) {
					self.cachedConnectors.push(connector);
				}
			});
		});
	}
	return this.cachedConnectors;
};

ms.boundNet.prototype.getBoundaryMorphism = function() {
	var indices = { halfs: [], vertices: [], faces: []};
	var self = this;
	var faces = this.getBoundary().getFaces();

	/* if (faces.length > 1) {
		faces = faces.filter((f) => f.getOuterComponent().getForward());
	} */
	var interior = this.getInterior();
	var interiorHalfs = interior.getHalfEdges();
	var interiorVerts = interior.getVertices();
	indices.faces = this.getOuterFaces(true).map((outerFace) => (
		interior.getFaces().indexOf(outerFace)
	));
	faces.forEach((face) => {
		var outerHalfs = face ? face.getOuterHalfEdges() : [];
		outerHalfs.forEach((half) => {
			var halfI = half.boundaryToInterior();
			indices.halfs.push(interiorHalfs.indexOf(halfI));
			if (half.getNext() == half && !half.getVertex()) {
				// This is an outer face with no connectors.
				return;
			}
			var vIndex = interiorVerts.indexOf(halfI.getVertex());
			if (!indices.vertices.includes(vIndex)) {
				indices.vertices.push(vIndex);
			}
		});
	});
	return indices;
};

// Remove any spliced edges. They should only appear on the interior network.
// TODO: We could probably remove this from every transition once rather than every time.
ms.boundNet.prototype.removeSplices = function() {
	var hasSplices = this.getInterior().getHalfEdges().some((half) => {
		return half.isSpliced();
	});
	if (!hasSplices) {
		return this;
	}
	
	var result = this.copy();
	// This is dangerous. We are making sure they are cached here.
	// If the cache is clear, we will get in an infinite loop in getConnectors.
	result.getConnectors();
	var halfsToRemove = [];
	var edgesToRemove = [];
	var interior = result.getInterior();
	interior.getHalfEdges().slice().forEach((half) => {
		var next = half.getNext();
		if (!half.isSpliced() && next && next.isSpliced()) {
			var newNext = next.getTwin().getNext();
			half.getEdge().merge(newNext.getEdge(), half.getForward());
		}
	});
	var hasSplices = interior.getHalfEdges().slice().forEach((half) => {
		if (half.isSpliced()) {
			interior.removeHalfEdge(half);
			var vertex = half.getVertex();
			if (vertex.inNetwork()) {
				interior.removeVertex(vertex);
			}
			var edge = half.getEdge();
			if (edge.inNetwork()) {
				interior.removeEdge(edge);
			}
		}
	});
	
	return result;
};

ms.boundNet.prototype.highlight = function(view, opt_options) {
	this.interior.highlight(view, opt_options);
};

ms.boundNet.prototype.draw = function(view, options) {
	this.interior.draw(view, options);
};

ms.boundNet.prototype.recomputeTurns = function() {
	var faces = this.getInterior().getFaces();
	faces.forEach((face) => { face.getPrimal().computeTurns(); });
};

ms.boundNet.prototype.getOuterFaces = function(allowEmptyFaces) {
	var interior = this.getInterior();
	var faces = interior.getFaces();
	if (allowEmptyFaces && interior.getVertices().length == 0 && interior.getEdges().length == 0) {
		return faces;
	}
	return faces.filter((face) => { return face.getPrimal().getTurns() == 1 && face.isLoopy(); });
};

ms.boundNet.prototype.print = function() {
	ms.highlight(this.interior);
};

ms.counter.register('boundNet');