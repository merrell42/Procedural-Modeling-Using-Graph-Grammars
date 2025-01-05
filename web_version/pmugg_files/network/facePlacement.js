ms.facePlacement = function(normal, id, settings, face) {
	this.normal = normal;
	this.face = face;
	this.id = id;
	this.settings = settings;
	this.free = true;
	this.vertexIds = [];
	// Already existing neighboring faces that are fixed to a position.
	this.fixedNeighbors = [];
	this.d = 0;
	this.fixed = false;
	this.slope = null;
	this.value = null;
};

ms.facePlacement.prototype.getFace = function() {
	return this.face;
};

// d is the position of the plane. The plane is defined as ax + by = cz = d.
ms.facePlacement.prototype.getD = function() {
	return this.d;
};

ms.facePlacement.prototype.isFree = function() {
	return this.free;
};

ms.facePlacement.prototype.getNormal = function() {
	return this.normal;
};

ms.facePlacement.prototype.setFixed = function(fixed) {
	this.fixed = fixed;
};

ms.facePlacement.prototype.getFixed = function() {
	return this.fixed;
};

ms.facePlacement.prototype.addVertexId = function(id) {
	if (!this.vertexIds.includes(id)) {
		this.vertexIds.push(id);
	}
};

ms.facePlacement.prototype.addFixedNeighbor = function(neighbor) {
	ms.union(this.fixedNeighbors, [neighbor]);
};

ms.facePlacement.prototype.coplanar = function(fPlaceB) {
	var nA = this.getNormal();
	var nB = fPlaceB.getNormal();
	return Math.abs(nA.dot(nB)) > 1 - 1e-4;
};

ms.facePlacement.prototype.constrain = function(addBasis, vertexId) {
	var { settings } = this;
	settings.addToOrder(this.id, 'face', vertexId);

	if (addBasis) {
		settings.basisIds.push(this.id);
	}
	this.free = false;
	var self = this;
	this.vertexIds.forEach((id) => {
		var vPlace = settings.getVertex(id);
		vPlace.constrainFace(self.id);
	});
	this.vertexIds.forEach((id) => {
		settings.getVertex(id).propagate();
	});
};

ms.facePlacement.prototype.setD = function(d) {
	this.d = d;
	this.slope = 0;
	this.value = d;
};

ms.facePlacement.prototype.setFromVertex = function(vertexId) {
	var { settings } = this;
	var n = this.getNormal();
	var vPlace = settings.getVertex(vertexId);	
	var d = n.dot(vPlace.getPosition());
	if (this.fixed && Math.abs(d - this.d) > 1e-4) {
	 	return false;
	}
	this.setD(d);
	return true;
};

ms.facePlacement.prototype.makeFixed = function(faceA) {
	var { settings } = this;
	var self = this;
	this.vertexIds.forEach((id) => {
		settings.getVertex(id).addFixedNeighbor(faceA);
	});
	this.constrain(true, -1);
	faceA.faceA.getGroup().connectHole(this.face.getGroup());
};

ms.facePlacement.prototype.getRange = function(vertexId) {
	var { settings } = this;
	var {lower, upper} = settings;

	var n = this.getNormal();
	var m = 1;
	var b = 0;
	if (vertexId !== -1) {
		var vPlace = settings.getVertex(vertexId);
		m = n.dot(vPlace.slope);
		b = n.dot(vPlace.value);
	}

	if (this.fixed) {
		if (m == 0) {
			// The vertex is already fixed. This face has no effect on it.
			return new ms.range(-Infinity, Infinity);
		} else {
			return new ms.range(this.value, this.value);
		}
	}
	this.slope = m;
	this.value = b;

	var lowD = 0;
	var highD = 0;
	for (var i = 0; i < 3; i++) {
		var ni = n.getValue(i);
		if (ni > 0) {
			lowD  += ni * lower[i];
			highD += ni * upper[i];
		} else {
			lowD  += ni * upper[i];
			highD += ni * lower[i];
		}
	}
	var range = new ms.range(lowD, highD);
	this.fixedNeighbors.forEach((neighbor) => {
		var rangeI = neighbor.dirBounds(n);
		range = range.intersect(rangeI);
	});
	return ms.range.transformCreate(m, b, range);
};

ms.facePlacement.prototype.getChangeMB = function() {
	return {m: this.slope, b: this.value};
};

ms.facePlacement.prototype.print = function() {
	this.face.print();
};
