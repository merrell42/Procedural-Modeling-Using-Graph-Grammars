ms.cell = function(id, position, isBoundary, model) {
	this.id = id;
	this.position = position;
	this.isBoundary = isBoundary;
	this.model = model;
	this.neighbors = [];
	this.mutable = false;
	this.mutationBorder = null;
	for (var i = 0; i < 2 * ms.cell.DIMS; i++) {
		this.neighbors.push(null);
	}
	this.node = null;
};

ms.cell.MAIN_DIRECTIONS = [
	[0, 0, 0],
	[1, 0, 0],
	[0, 0, 0],
	[0, 1, 0],
	[0, 0, 0],
	[0, 0, 1]
];

ms.cell.BOUNDARY_DIRECTIONS = [
	[-1,  0,  0],
	[ 0,  0,  0],
	[ 0, -1,  0],
	[ 0,  0,  0],
	[ 0,  0, -1],
	[ 0,  0,  0],
];

ms.cell.DIRECTIONS = [
	[-1,  0,  0],
	[ 1,  0,  0],
	[ 0, -1,  0],
	[ 0,  1,  0],
	[ 0,  0, -1],
	[ 0,  0,  1]
];

ms.cell.DIRECTIONS_VEC = [
	new ms.vec2(-1,  0),
	new ms.vec2( 1,  0),
	new ms.vec2( 0, -1),
	new ms.vec2( 0,  1),
	new ms.vec2( 0,  0),
	new ms.vec2( 0,  0)
];

ms.cell.PERPENDICULAR_VEC = [
	new ms.vec2( 0, 1),
	new ms.vec2( 0, 1),
	new ms.vec2( 1, 0),
	new ms.vec2( 1, 0),
	new ms.vec2( 0, 0),
	new ms.vec2( 0, 0)
];

ms.cell.DIMS = 3;

ms.cell.oppositeDir = function(dir) {
	return dir ^ 1;
};

ms.cell.getDirectionIndex = function(direction) {
	if (direction[0] != 0) {
		return (direction[0] < 0) ? 0 : 1;
	} else if (direction[1] != 0) {
		return (direction[1] < 0) ? 2 : 3;
	} else if (direction[2] != 0) {
		return (direction[2] < 0) ? 4 : 5;
	}
	return -1;
};

// Returns a vector with the given angle towards a direction.
ms.cell.vecInDirection = function(angle, dir) {
	var u = ms.vec2.unitVec(angle);
	if (u.dot(ms.cell.DIRECTIONS_VEC[dir]) < 0) {
		u.scale(-1);
	}
	return u;
};

ms.cell.prototype.setNodeStats = function(stats) {
	var properties = {
		faceConnection: new ms.alternativeArray('faceConnection', /* required */ false),
		lineState: new ms.alternativeArray('lineState', /* required */ false),
	};
	this.node = new ms.node(this, stats, 'cell', properties);
};

ms.cell.prototype.getNode = function() {
	return this.node;
};

// TODO: Rename to just getStates.
ms.cell.prototype.getActiveStates = function() {
	return this.node ? this.node.get('lineState') : [];
};

ms.cell.prototype.getFaceConnections = function() {
	return this.node ? this.node.get('faceConnection') : [];
};

ms.cell.prototype.getPosition = function() {
	return this.position;
};

ms.cell.prototype.getDirection = function(dir) {
	if (ms.globalSettings.get('Use Boundary Cells')) {
		return this.isBoundary ? ms.cell.BOUNDARY_DIRECTIONS[dir] : ms.cell.MAIN_DIRECTIONS[dir];
	} else {
		return ms.cell.DIRECTIONS[dir];
	}
};

ms.cell.prototype.getIsBoundary = function() {
	return this.isBoundary;
};

ms.cell.prototype.isMutable = function() {
	return this.mutable;
};

ms.cell.prototype.getModel = function() {
	return this.model;
};

ms.cell.prototype.restrain = function() {
	this.mutable = false;
};

ms.cell.prototype.free = function() {
	this.mutable = true;
};

ms.cell.prototype.save = function() {};

ms.cell.prototype.addNeighbor = function(dir, neighbor) {
	this.neighbors[dir] = neighbor;
};

ms.cell.prototype.getNeighbor = function(dir) {
	return this.neighbors[dir];
};

ms.cell.prototype.addState = function(state) {
	this.node.connect(state);
};

// Removes a state.
ms.cell.prototype.removeState = function(state) {
	this.node.disconnect(state);
};

ms.cell.prototype.addFaceConnection = function(connection) {
	this.node.connect(connection);
};

// Removes a face connection.
ms.cell.prototype.removeFaceConnection = function(connection) {
	this.node.disconnect(connection);
};

ms.cell.prototype.getVertex = function() {
	var endpoints = this.getActiveStates()[0].getLine().getEndpoints();
	var endpoint = (endpoints[0].getVertex().getCell() == this) ? endpoints[0] : endpoints[1];
	return endpoint.getVertex();
};

ms.cell.prototype.setMutationBorder = function(mutationBorder) {
	this.mutationBorder = mutationBorder;
};

ms.cell.prototype.getMutationBorder = function() {
	return this.mutationBorder;
};

ms.cell.prototype.draw = function(context, xOffset, yOffset) {};

ms.cell.prototype.print = function(opt_category) {
	var category = opt_category || 'default';
	window.console.log(this.id);
	var states = this.getActiveStates(category);
	if (!states) {
		window.console.log('Empty');
		return;
	}
	for (var i = 0; i < states.length; i++) {
		states(category)[i].print();
	}
};
