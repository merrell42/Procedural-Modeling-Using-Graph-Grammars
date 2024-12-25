ms.vertexPlacement = function(vertex, id, settings) {
	this.vertex = vertex;
	this.id = id;
	this.settings = settings;
	this.freeFaceIds = [];
	this.unfreeFaceIds = [];
	// This ID depends on the others. It cannot be used as one of the three unfreeFaceIds.
	this.colinearFaceIds = [];

	this.M = null;

	// How the position varies with the current range we are examining.
	// When the vertex position is set, these should be null and position should be used
	this.slope = null;
	this.value = null;
};

ms.vertexPlacement.prototype.initialize = function() {
	var endpoints = this.vertex.getEndpoints();
	var self = this;
	var { settings } = this;
	endpoints.forEach((endpoint) => {
		var face = endpoint.getFace();
		var id = face.getNode().id;
		if (!settings.getFace(id)) {
			var normal = face.getFaceType().getNormal();
			settings.facePlacements[id] = new ms.facePlacement(normal, id, settings, face);
		}
		this.addFreeFace(id);
	});
};

// Add a fixed neighbor to all the faces.
ms.vertexPlacement.prototype.addFixedNeighbor = function(fixedFace) {
	var { settings } = this;
	this.getAllFaceIds().forEach((id) => {
		if (id != fixedFace.fPlace.id) {
			settings.getFace(id).addFixedNeighbor(fixedFace.faceA);
		}
	});
};

ms.vertexPlacement.prototype.addFreeFace = function(id) {
	var settings = this.settings;
	var newId = settings.uniqueFaceMap[id];
	if (newId !== undefined) { id = newId; }
	if (this.freeFaceIds.includes(id)) {
		return;
	}
	var newFreeFace = this.settings.getFace(id);
	var coplanarId = this.freeFaceIds.find((id) => {
		return newFreeFace.coplanar(settings.getFace(id));
	});
	if (coplanarId) {
		settings.mergeFace(coplanarId, id);
		id = coplanarId;
	} else {
		this.freeFaceIds.push(id);	
	}
	
	this.settings.getFace(id).addVertexId(this.id);
	
	var colinear = (this.freeFaceIds.length == 3 && !this.getA(this.freeFaceIds));
	if (colinear) {
		this.freeFaceIds.pop();
	}
};

// Check if there are three faces. Add one if there is not.
ms.vertexPlacement.prototype.checkThreeFaces = function(id) {
	var { settings } = this;
	if (this.freeFaceIds.length < 2) {
		ms.alert('Vertex should have at least two faces.');
	} else if (this.freeFaceIds.length == 2) {
		var n0 = settings.getFace(this.freeFaceIds[0]).getNormal();
		var n1 = settings.getFace(this.freeFaceIds[1]).getNormal();
		var n2 = n0.cross(n1);
		n2.normalize();
		var id = this.settings.createFace(n2);
		this.addFreeFace(id);
	}
};

ms.vertexPlacement.prototype.getNumConstraints = function() {
	return this.unfreeFaceIds.length;
};

ms.vertexPlacement.prototype.getPosition = function() {
	return this.value;
};

ms.vertexPlacement.prototype.getAllFaceIds = function() {
	return this.unfreeFaceIds.concat(this.freeFaceIds, this.colinearFaceIds);
};

ms.vertexPlacement.prototype.fixPosition = function() {
	var { settings } = this;
	this.slope = ms.vec3.ORIGIN;
	this.value = this.vertex.getPosition();
	var self = this;
	var success = true;
	this.getAllFaceIds().forEach((id) => {
		success = success && settings.getFace(id).setFromVertex(self.id);
		settings.getFace(id).setFixed(true);
	});
	return success;
};

ms.vertexPlacement.prototype.addConstraint = function() {
	var { settings } = this;
	var freeFaceId = this.freeFaceIds[0];
	settings.getFace(freeFaceId).constrain(true);
};

ms.vertexPlacement.prototype.constrainFace = function(id) {
	ms.remove(id, this.freeFaceIds);
	this.unfreeFaceIds.push(id);

	// Check if the face IDs are colinear.
	if (this.unfreeFaceIds.length == 3) {
		var M = this.getM();
		if (!M) {
			this.unfreeFaceIds.pop();
			this.colinearFaceIds.push(id);
		}
	}
};

ms.vertexPlacement.prototype.propagate = function(id) {
	var { settings } = this;
	var self = this;
	if (this.unfreeFaceIds.length >= 3) {
		settings.addToOrder(this.id, 'vertex');
		var freeIds = this.freeFaceIds.concat(this.colinearFaceIds).slice();
		freeIds.forEach((id) => {
			var fPlace = settings.getFace(id);
			if (fPlace.isFree()) {
				fPlace.constrain(false, this.id);
			}
		});
		this.vertex.getEndpoints().forEach((endpoint) => {
			var id = endpoint.getLine().getNode().getId()
			var ePlace = settings.getEdge(id);
			ePlace && ePlace.addConstraint(self.id);
			var prev = endpoint.prev();
			if (prev.getEdgeType().faceData.length == 1) {
				// If prev edge only has one endpoint. Propagate to previous vertex.				
				var id = prev.getLine().getNode().getId()
				var ePlace = settings.getEdge(id);
				ePlace && ePlace.addConstraint(self.id);
			}
		});
	}
};

// Ax = D where A is the normals. a_i *x + b_i * y + c_i * z = d_i.
// A^-1 * D = x. M = A^-1.
// Returns null, if A is not invertible.
ms.vertexPlacement.prototype.getA = function(faceIds) {
	var { settings } = this;
	var A = [];
	for (var i = 0; i < 3; i++) {
		var id = faceIds[i];
		var fPlace = settings.getFace(id);
		var n = fPlace.getNormal();
		A.push([n.x, n.y, n.z]);
	}
	return mathG.matrix(A);
};

// Ax = D where A is the normals. a_i *x + b_i * y + c_i * z = d_i.
// A^-1 * D = x. M = A^-1.
// Returns null, if A is not invertible.
ms.vertexPlacement.prototype.getM = function() {
	if (!this.M) {
		var A = this.getA(this.unfreeFaceIds);
		// The three faces are colinear.
		if (Math.abs(mathG.det(A)) < 1e-8) {
			return null;
		}
		this.M = mathG.inv(A);
	}
	return this.M;
};

// Vertex positions x = M * D.
ms.vertexPlacement.prototype.setPosition = function() {
	var { settings } = this;
	var D = [];
	for (var i = 0; i < 3; i++) {
		var id = this.unfreeFaceIds[i];
		var fPlace = settings.getFace(id);
		D.push([fPlace.getD()]);
	}
	D = mathG.matrix(D);
	var b = mathG.multiply(this.getM(), D).valueOf();
	this.slope = ms.vec3.ORIGIN;
	this.value = new ms.vec3(b[0][0], b[1][0], b[2][0]);
};

// Gets the range of possible values for the vertex.
ms.vertexPlacement.prototype.getRange = function() {
	var self = this;
	var { settings } = this;

	// Figure out how the three unfree faces vary.
	var m3 = [];
	var b3 = [];
	for (var i = 0; i < 3; i++) {
		var id = this.unfreeFaceIds[i];
		var fPlace = settings.getFace(id);
		var mbi = fPlace.getChangeMB();
		m3.push([mbi.m]);
		b3.push([mbi.b]);
	}
	m3 = mathG.matrix(m3);
	b3 = mathG.matrix(b3);
	var m = mathG.multiply(this.getM(), m3).valueOf();
	var b = mathG.multiply(this.getM(), b3).valueOf();

	var {lower, upper} = settings;
	var range = new ms.range(-Infinity, Infinity);
	for (var i = 0; i < 3; i++) {
		var rangeI = ms.range.transformCreate(m[i][0], b[i][0], new ms.range(lower[i], upper[i]));
		range = range.intersect(rangeI);
	}

	this.slope = new ms.vec3(m[0][0], m[1][0], m[2][0]);
	this.value = new ms.vec3(b[0][0], b[1][0], b[2][0]);

	return range;
};

ms.vertexPlacement.prototype.getChangeMB = function() {
	return {m: this.slope, b: this.value};
};

ms.vertexPlacement.prototype.print = function() {
	this.vertex.print();
};
