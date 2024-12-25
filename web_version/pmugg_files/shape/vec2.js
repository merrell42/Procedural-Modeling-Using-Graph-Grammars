// 2D Vectors
ms.vec2 = function(x, y) {
	this.x = x;
	this.y = y;
	this.z = 0;
};

ms.vec2.ORIGIN = new ms.vec2(0, 0);

ms.vec2.prototype.copy = function() {
	return new ms.vec2(this.x, this.y);
};

// Copy from a vector.
ms.vec2.prototype.set = function(a) {
  this.x = a.x;
  this.y = a.y;
};

ms.vec2.prototype.set2 = function(x, y) {
  this.x = x;
  this.y = y;
};

ms.vec2.prototype.distance2 = function(position) {
	var dx = this.x - position.x;
	var dy = this.y - position.y;
	return dx * dx + dy * dy;
};

ms.vec2.prototype.distance = function(position) {
	return Math.sqrt(this.distance2(position));
};

ms.vec2.prototype.length = function() {
	return Math.sqrt(this.x * this.x + this.y * this.y);
};

ms.vec2.prototype.length2 = function(position) {
	return this.x * this.x + this.y * this.y;
};

ms.vec2.prototype.add = function(position) {
	this.x += position.x;
	this.y += position.y;
	return this;
};

ms.vec2.prototype.minus = function(position) {
	this.x -= position.x;
	this.y -= position.y;
	return this;
};

ms.vec2.prototype.normalize = function() {
	var r2 = this.x * this.x + this.y * this.y;
	if (r2 != 0) {
		this.scale(1 / Math.sqrt(r2));
	}
	return this;
};

ms.vec2.prototype.scale = function(s) {
	this.x *= s;
	this.y *= s;
	return this;
};

ms.vec2.lerp = function(start, end, s) {
	var x = (1 - s) * start.x + s * end.x;
	var y = (1 - s) * start.y + s * end.y;
	return new ms.vec2(x, y);
};

// Compute the dot product.
ms.vec2.prototype.dot = function(v) {
	return this.x * v.x + this.y * v.y;
};

ms.vec2.prototype.print = function(v) {
	return '(' + parseFloat(this.x).toFixed(3) + ', ' + parseFloat(this.y).toFixed(3) + ')';
};

ms.vec2.prototype.cross = function(b) {
	var a = this;
	if (a.z != 0 || b.z != 0) {
		ms.alert('Not 2D vectors.');
	}
	return new ms.vec3(0, 0, a.x * b.y - a.y * b.x);
};

// 2D cross product. Magnitude of the cross product in the z-direction.
ms.vec2.prototype.crossZ = function(v) {
  return this.x * v.y - this.y * v.x;
};

// Get the value along a particular dimension.
ms.vec2.prototype.getValue = function(dim) {
  if (dim == 0) {
    return this.x;
  } else if (dim == 1) {
    return this.y; 
  } else {
    alert('Get value undefined for dimension' + dim);
  }
};

// Get the value along a particular dimension.
ms.vec2.prototype.setValue = function(value, dim) {
  if (!dim) {
    this.x = value;
  } else {
    this.y = value;
  }
};


// Determines the xy lexicographical order of two points for sweepline.
ms.vec2.prototype.compare = function(other_point){
  // x-coord first
  if (this.x > other_point.x) return  1; 
  if (this.x < other_point.x) return -1;

  // y-coord second
  if (this.y > other_point.y) return  1; 
  if (this.y < other_point.y) return -1;

  // they are the same point
  return 0;  
};

ms.vec2.rotationMatrix = function(theta) {
	var cosTheta = Math.cos(theta);
	var sinTheta = Math.sin(theta);
	return [[cosTheta, -sinTheta], [sinTheta, cosTheta]];
};

ms.vec2.prototype.matrixMultiply = function(A) {
	var x = A[0][0] * this.x + A[0][1] * this.y;
	var y = A[1][0] * this.x + A[1][1] * this.y;
	this.x = x;
	this.y = y;
};

ms.vec2.prototype.rotate = function(theta) {
	var R = ms.vec2.rotationMatrix(theta);
	this.matrixMultiply(R);
	return this;
};

// tests if point is Left|On|Right of the line P0 to P1.
//
// returns: 
//  >0 for left of the line 
//  0 for on the line
//  <0 for right of the line
ms.vec2.prototype.is_left = function(p0, p1){
 return (p1.x - p0.x) * (this.y - p0.y) - (this.x - p0.x) * (p1.y - p0.y);  
}

ms.vec2.coordinatesClose = function(a, b, tolerance) {
	return (Math.abs(a.x - b.x) <= tolerance && Math.abs(a.y - b.y) <= tolerance);
};

ms.vec2.prototype.coordinatesClose = function(b, tolerance) {
	return ms.vec2.coordinatesClose(this, b, tolerance);
};

ms.vec2.unitVec = function(angle) {
	return new ms.vec2(Math.cos(angle), Math.sin(angle));
};

ms.vec2.angle = function(startPos, endPos) {
	var dx = endPos.x - startPos.x;
	var dy = endPos.y - startPos.y;
	return Math.atan2(dy, dx);
};

ms.vec2.prototype.move = function(dx, dy) {
	this.x += dx;
	this.y += dy;
};

ms.vec2.prototype.dropDim = function() {
	return this.copy();
};

// Convert from a vector to a matrix.
ms.vec2.prototype.toMatrix = function() {
	return mathG.matrix([[this.x], [this.y]]);
};

ms.vec2.prototype.export = function() {
	return { x: this.x, y: this.y, z: this.z };
};

ms.vec2.import = function(json) {
	return new ms.vec3(json.x, json.y, json.z);
};
