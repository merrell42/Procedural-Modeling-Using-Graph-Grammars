ms.edgeType = function(isRigid, isRigidTiled, brush, edgeLength, angle, offset) {
	this.leftArea = '';
	this.rightArea = '';
	this.isRigid = isRigid;
	this.isRigidTiled = isRigidTiled;
	this.edgeLength = edgeLength;
	this.angle = angle;
	this.offset = offset;
	this.brush = brush;

	this.id = ms.edgeType.count++;
};

ms.edgeType.count = 0;

ms.edgeType.prototype.export = function(types) {
	var result = {...this};
	result.brush = this.brush && this.brush.export && this.brush.export();
	result.leftArea = this.leftArea ? types.faceTypes.indexOf(this.leftArea) : -1;
	result.rightArea = this.rightArea ? types.faceTypes.indexOf(this.rightArea) : -1;
	if ((this.leftArea && result.leftArea == -1) ||
		(this.rightArea && result.rightArea == -1)) {
		console.log('Areas may not be exported correctly in edgeType.');
	}
	return result;
};

ms.edgeType.import = function(json, types) {
	var brush = json.brush ? ms.brush.import(5, [json.brush], () => {}) : null;
	var offset = json.offset && ms.vec2.import(json.offset);
	var result = new ms.edgeType(json.isRigid, json.isRigidTiled, brush, json.edgeLength, json.angle, offset);
	result.leftArea = (json.leftArea == -1) ? null : types.faceTypes[json.leftArea];
	result.rightArea = (json.rightArea == -1) ? null : types.faceTypes[json.rightArea];
	result.id = json.id;
	return result;
};

ms.edgeType.prototype.is3D = function() {
	return false;
};

ms.edgeType.prototype.addStartVertex = function(v, angle) {
	v.addEdge(this, true, angle);
};

ms.edgeType.prototype.addEndVertex = function(v, angle) {
	v.addEdge(this, false, angle);
};

ms.edgeType.prototype.getBrush = function() {
	return this.brush;
};

ms.edgeType.prototype.getAngle = function() {
	return this.angle;
};

ms.edgeType.prototype.setAngle = function(angle) {
	this.angle = angle;
};

ms.edgeType.prototype.getDir = function() {
	return ms.vec2.unitVec(this.angle);
};

ms.edgeType.prototype.getId = function() {
	return this.id;
};

ms.edgeType.prototype.getOffset = function() {
	return this.offset;
};

ms.edgeType.prototype.extendable = function() {
	return !this.isRigid || this.isRigidTiled;
};

// TODO: This shouldn't be needed.
ms.edgeType.prototype.getEdgeType = function() {
	return this;
};

ms.edgeType.prototype.getIsRigid = function() {
	return this.isRigid;
};

ms.edgeType.prototype.getEdgeLength = function() {
	return this.edgeLength;
};

ms.edgeType.prototype.setAreas = function(rightArea, leftArea) {
	this.leftArea  = leftArea;
	this.rightArea = rightArea;
};

ms.edgeType.prototype.getLeftArea = function() {
	return this.leftArea;
};

ms.edgeType.prototype.getRightArea = function() {
	return this.rightArea;
};

ms.edgeType.prototype.getThickness = function() {
	return this.brush ? this.brush.get('Thickness') : ms.lineState.INTERSECTION_THICKNESS;
};

ms.edgeType.prototype.isLoopy = function() {
	return this.brush ? this.brush.get('Loopy') : true;
};

ms.edgeType.prototype.isBoundary = function() {
	return this.brush ? this.brush.get('Boundary') : false;
};

ms.edgeType.prototype.isConnected = function() {
	return this.brush ? this.brush.get('Fully Connected') : false;
};

// If production rules that split a fragment into multiple fragment is not allowed.
// If there is one connected component, there remains one connected component.
// I may add other single fragments besides the boundary.
ms.edgeType.prototype.singleFragment = function() {
	return this.isBoundary();
};

// If production rules that split a fragment into multiple fragment is not allowed.
ms.edgeType.prototype.splittable = function() {
	return !(!this.isLoopy() || this.isBoundary() || this.isConnected());
};

ms.edgeType.prototype.isGroundType = function() {
	return this.brush && this.brush.get('Grounded') &&
		(!this.leftArea  || this.leftArea.isGround) &&
		(!this.rightArea || this.rightArea.isGround);
};

ms.edgeType.prototype.getFaceData = function() {
	return [];
};
