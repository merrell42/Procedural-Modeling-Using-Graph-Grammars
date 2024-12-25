ms.edgeType3D = function(faceData, dir, opt_options) {
	// Data consists of {type: faceType3D, onRight: boolean}.
	this.faceData = faceData;
	this.dir = dir;
	
	var options = opt_options || {};

	this.isRigid = options.isRigid || false;
	this.isRigidTiled = options.isRigidTiled || false;
	this.edgeLength = options.edgeLength || Infinity;
	this.angle = options.angle || ms.vec2.angle(ms.vec2.ORIGIN, this.dir);
	this.offset = options.offset || null;
	this.brush = options.brush || null;
	this.monotonic = null;
	// this.is3D = true;
	this.spliced = false;
	// Destroyed means this is part of a super edge and is not part of the gluing options.
	this.destroyed = false;
	
	this.id = ms.counter.add('edgeType3D');
};

ms.counter.register('edgeType3D');

ms.edgeType3D.prototype.export = function(types) {
	return {
		faceData: this.faceData.map((f) => { return { type: types.faceTypes.indexOf(f.type), onRight: f.onRight }}),
		// This might be more complicated for 2D brushes.
		brush: this.brush && this.brush.export && this.brush.export(),

		dir: this.dir.export(),
		isRigid: this.isRigid,
		isRigidTiled: this.isRigidTiled,
		edgeLength: this.edgeLength,
		angle: this.angle,
		offset: this.offset,
		isRigid: this.isRigid,
		spliced: this.spliced,
	};
};

ms.edgeType3D.import = function(json, types) {
	var faceData = json.faceData.map((f) => ({
		type: types.faceTypes[f.type],
		onRight: f.onRight,
	}));	
	var dir = ms.vec3.import(json.dir);
	var options = {...json};
	options.brush = options.brush && (ms.brush.import(5, [options.brush], () => {}));
	var result = new ms.edgeType3D(faceData, dir, options);
	result.setSpliced(json.spliced);
	return result;
};

ms.edgeType3D.partialImport = function(json, faceTypes) {
	var faceData = json.faceData.map((f) => ({
		type: faceTypes[f.type],
		onRight: f.onRight,
	}));	
	var dir = ms.vec3.import(json.dir).swapAxes();	
	var brush = new ms.brush('#000', '#000', () => {})
	var result = new ms.edgeType3D(faceData, dir, {brush: brush});
	result.id = json.id;
	result.idNum = json.idNum;
	return result;
};

ms.edgeType3D.prototype.is3D = function() {
	// return this.is3D;
	// Something is not right.
	return false;
};

ms.edgeType3D.prototype.getDir = function() {
	return this.dir;
};

ms.edgeType3D.prototype.getFaceData = function() {
	return this.faceData;
};

// Angles are not really relevant for 3D edges.
// Use direction when possible.
ms.edgeType3D.prototype.getAngle = function() {
	return this.angle;
};

ms.edgeType3D.prototype.setAngle = function(angle) {
	this.angle = angle;
};

ms.edgeType3D.prototype.setSpliced = function(spliced) {
	this.spliced = spliced;
	if (spliced) {
		this.brush = {getColor: () => '#aaa'};
	}
	return this;
};

ms.edgeType3D.prototype.getSpliced = function() {
	return this.spliced;
};

ms.edgeType3D.prototype.isDestroyed = function() {
	return this.destroyed;
};

ms.edgeType3D.prototype.destroy = function() {
	this.destroyed = true;
};

ms.edgeType3D.prototype.getBrush = function() {
	return this.brush;
};

ms.edgeType3D.prototype.isLoopy = function() {
	return this.brush ? this.brush.get('Loopy') : true;
};

ms.edgeType3D.prototype.isBoundary = function() {
	return this.brush ? this.brush.get('Boundary') : false;
};

ms.edgeType3D.prototype.isConnected = function() {
	if (this.spliced) { return false; }
	return this.brush ? this.brush.get('Fully Connected') : false;
};

ms.edgeType3D.prototype.getThickness = function() {
	return this.brush ? this.brush.get('Thickness') : ms.lineState.INTERSECTION_THICKNESS;
};

// If production rules that split a fragment into multiple fragment is not allowed.
// If there is one connected component, there remains one connected component.
// I may add other single fragments besides the boundary.
ms.edgeType3D.prototype.singleFragment = function() {
	return this.isBoundary();
};

// If production rules that split a fragment into multiple fragment is not allowed.
ms.edgeType3D.prototype.splittable = function() {
	return !(!this.isLoopy() || this.isBoundary() || this.isConnected());
};

ms.edgeType3D.prototype.extendable = function() {
	return !this.isRigid || this.isRigidTiled;
};

ms.edgeType3D.prototype.getEdgeLength = function() {
	return this.edgeLength;
};

ms.edgeType3D.prototype.getIsRigid = function() {
	return this.isRigid;
};

ms.edgeType3D.prototype.getMonotonic = function() {
	return this.monotonic;
};

ms.edgeType3D.prototype.setMonotonic = function(monotonic) {
	this.monotonic = monotonic;
};

ms.edgeType3D.prototype.getId = function() {
	return this.id;
};

ms.edgeType3D.prototype.getLeftArea = function() {
	var leftDatum = this.faceData.find((faceDatum) => { return faceDatum.onRight == false; });
	return leftDatum && leftDatum.type.getMaterial();
};

ms.edgeType3D.prototype.getRightArea = function() {
	var rightDatum = this.faceData.find((faceDatum) => { return faceDatum.onRight == true; });
	return rightDatum && rightDatum.type.getMaterial();
};

// Find the neighboring face that shares a volume with the one at initialIndex.
// above means we are sweeping in the normal direction of initialIndex.
ms.edgeType3D.prototype.neighboringFace = function(initialIndex, above) {
	var maxDim = ms.util.maxDim(this.dir);
	var angles = [];
	this.faceData.map((faceDatum, index) => {
		var x, y;
		var v = this.dir.cross(faceDatum.type.normal);
		if (!faceDatum.onRight) {
			v.scale(-1);
		}
		switch(maxDim) {
			case 0: x = v.x; y = v.y; break;
			case 1: x = v.z; y = v.x; break;
			case 2: x = v.y; y = v.z; break;
		}
		angles.push([Math.atan2(x, y), index]);
	});
	angles.sort((a, b) => { return a[0] > b[0]; });
	var fOrder = angles.findIndex((angle) => angle[1] == initialIndex);
	var neighborOrder = (fOrder + (above ? 1 : -1) + angles.length) % angles.length;
	var neighborIndex = angles[neighborOrder][1];
	return neighborIndex;

	// neighborAbove isn't necessary to calculate. This hasn't been tested.
	/* var initial  = this.faceData[initialIndex];
	var neighbor = this.faceData[neighborIndex]; 
	var n         = initial.type.normal;
	var nNeighbor = neighbor.type.normal;
	var neighborAbove = (n.dot(nNeighbor) < 0) ^ initial.onRight ^ nNeighbor.onRight;
	return {index: neighborIndex, above: neighborAbove}; */
};

/* ms.edgeType3D.prototype.addGraphEndpoints = function(edge, vertex0, vertex1) {
	this.faceData.forEach(function(faceDatum, index) {
		// var vertex = faceDatum.onRight ? vertex0 : vertex1;
		var vertex = vertex0;
		var endpoint = new ms.graphEndpoint(vertex, edge, faceDatum.type);
		edge.setEndpoint(endpoint, index);
		vertex.addEndpoint(endpoint);
	});
}; */

ms.edgeType3D.prototype.boundaryString = function() {
	if (ms.vec3.X_HAT.dot(this.dir) > 0.99) {
		return 'x';
	} else if (ms.vec3.Y_HAT.dot(this.dir) > 0.99) {
		return 'y';
	} else if (ms.vec3.Z_HAT.dot(this.dir) > 0.99) {
		return 'z';
	}
	return String.fromCharCode(this.id + 'a'.charCodeAt(0))
};

// Copied from 2D edgeType. Probably unnecessary
ms.edgeType3D.prototype.addStartVertex = function(v, angle) {
	v.addEdge(this, true, angle);
};

ms.edgeType3D.prototype.addEndVertex = function(v, angle) {
	v.addEdge(this, false, angle);
};
