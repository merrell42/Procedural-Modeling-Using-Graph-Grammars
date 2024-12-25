ms.angledEdge = function(position, angle, t) {
	this.position = position;
	this.angle = angle;
	// T is the coordinate along the direction of the edge tiling.
	this.t = t;
};

ms.angledEdge.RAY_LENGTH = 1000;

ms.angledEdge.prototype.copy = function() {
	return new ms.angledEdge(this.position.copy(), this.angle, this.t);
};

ms.angledEdge.prototype.getPosition = function() {
	return this.position;
};

ms.angledEdge.prototype.setPosition = function(position) {
	this.position = position;
};

ms.angledEdge.prototype.set = function(crossing) {
	this.position = crossing.getPosition();
	this.angle = crossing.getAngle();
};

ms.angledEdge.prototype.setAngle = function(angle) {
	this.angle = angle;
};

ms.angledEdge.prototype.getAngle = function() {
	return this.angle;
};

ms.angledEdge.prototype.setT = function(t) {
	this.t = t;
};

ms.angledEdge.prototype.getT = function() {
	return this.t;
};

ms.angledEdge.prototype.print = function() {
	return this.position.print();
};

ms.angledEdge.prototype.move = function(motion) {
	this.position.add(motion);
};
