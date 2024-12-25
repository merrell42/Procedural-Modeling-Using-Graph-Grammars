// 3D Vectors
ms.vec3 = function(x, y, z) {
	this.x = x;
	this.y = y;
	this.z = z;
};

ms.vec3.ORIGIN = new ms.vec3(0, 0, 0);
ms.vec3.X_HAT = new ms.vec3(1, 0, 0);
ms.vec3.Y_HAT = new ms.vec3(0, 1, 0);
ms.vec3.Z_HAT = new ms.vec3(0, 0, 1);

ms.vec3.create = function(v) {
	return new ms.vec3(v.x, v.y, v.z);
};

ms.vec3.prototype.copy = function() {
	return new ms.vec3(this.x, this.y, this.z);
};

ms.vec3.prototype.toArray = function() {
	return [this.x, this.y, this.z];
};

// Copy from a vector.
ms.vec3.prototype.set = function(a) {
  this.x = a.x;
  this.y = a.y;
  this.z = a.z;
};

ms.vec3.prototype.set3 = function(x, y, z) {
  this.x = x;
  this.y = y;
  this.z = z;
};

ms.vec3.prototype.distance2 = function(position) {
	var dx = this.x - position.x;
	var dy = this.y - position.y;
	var dz = this.z - position.z;
	return dx * dx + dy * dy + dz * dz;
};

ms.vec3.prototype.distance = function(position) {
	return Math.sqrt(this.distance2(position));
};

ms.vec3.prototype.length = function() {
	return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
};

ms.vec3.prototype.length2 = function(position) {
	return this.x * this.x + this.y * this.y  + this.z * this.z;
};

ms.vec3.prototype.add = function(position) {
	this.x += position.x;
	this.y += position.y;
	this.z += position.z;
	return this;
};

ms.vec3.prototype.minus = function(position) {
	this.x -= position.x;
	this.y -= position.y;
	this.z -= position.z;
	return this;
};

ms.vec3.prototype.normalize = function() {
	var r2 = this.x * this.x + this.y * this.y + this.z * this.z;
	if (r2 != 0) {
		this.scale(1 / Math.sqrt(r2));
	}
	return this;
};

ms.vec3.prototype.scale = function(s) {
	this.x *= s;
	this.y *= s;
	this.z *= s;
	return this;
};

ms.vec3.lerp = function(start, end, s) {
	var x = (1 - s) * start.x + s * end.x;
	var y = (1 - s) * start.y + s * end.y;
	var z = (1 - s) * start.z + s * end.z;
	return new ms.vec3(x, y, z);
};

// Compute the dot product.
ms.vec3.prototype.dot = function(v) {
	return this.x * v.x + this.y * v.y + this.z * v.z;
};

ms.vec3.prototype.print = function(v) {
	return '(' + parseFloat(this.x).toFixed(3) + ', ' + parseFloat(this.y).toFixed(3) + ', ' + parseFloat(this.z).toFixed(3) + ')';
};

ms.vec3.prototype.cross = function(b) {
	var a = this;
	return new ms.vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
};

// Magnitude of the cross product in the z-direction.
ms.vec3.prototype.crossZ = function(v) {
  return this.x * v.y - this.y * v.x;
};

// Get the value along a particular dimension.
ms.vec3.prototype.getValue = function(dim) {
  if (dim == 0) {
    return this.x;
  } else if (dim == 1) {
    return this.y; 
  } else if (dim == 2) {
    return this.z; 
  } else {
    alert('Get value undefined for dimension' + dim);
  }
};

// Get the value along a particular dimension.
ms.vec3.prototype.setValue = function(value, dim) {
  if (dim == 0) {
    this.x = value;
  } else if (dim == 1) {
    this.y = value;
  } else {
    this.z = value;	  
  }
};

// Drop one of the dimensions. Returns a vec2 with it missing.
ms.vec3.prototype.dropDim = function(dim) {
	if (dim == 0) {
		return new ms.vec2(this.y, this.z);
	} else if (dim == 1) {
		return new ms.vec2(this.x, this.z);
	} else {
		return new ms.vec2(this.x, this.y);
	}
};


// Determines the xy lexicographical order of two points for sweepline.
/* ms.vec3.prototype.compare = function(other_point){
  // x-coord first
  if (this.x > other_point.x) return  1; 
  if (this.x < other_point.x) return -1;

  // y-coord second
  if (this.y > other_point.y) return  1; 
  if (this.y < other_point.y) return -1;

  // they are the same point
  return 0;  
}; */

ms.vec3.rotationMatrix = function(theta) {
	var cosTheta = Math.cos(theta);
	var sinTheta = Math.sin(theta);
	return [[cosTheta, -sinTheta, 0], [sinTheta, cosTheta, 0], [0, 0, 1]];
};

ms.vec3.prototype.matrixMultiply = function(A) {
	var x = A[0][0] * this.x + A[0][1] * this.y + A[0][2] * this.z;
	var y = A[1][0] * this.x + A[1][1] * this.y + A[1][2] * this.z;
	var z = A[2][0] * this.x + A[2][1] * this.y + A[2][2] * this.z;
	this.x = x;
	this.y = y;
	this.z = y;
};

ms.vec3.prototype.rotate = function(theta) {
	var R = ms.vec3.rotationMatrix(theta);
	this.matrixMultiply(R);
	return this;
};

/*
// tests if point is Left|On|Right of the line P0 to P1.
//
// returns: 
//  >0 for left of the line 
//  0 for on the line
//  <0 for right of the line
ms.vec3.prototype.is_left = function(p0, p1){
 return (p1.x - p0.x) * (this.y - p0.y) - (this.x - p0.x) * (p1.y - p0.y);  
} */

ms.vec3.coordinatesClose = function(a, b, tolerance) {
	return (Math.abs(a.x - b.x) <= tolerance &
		Math.abs(a.y - b.y) <= tolerance & 
		Math.abs(a.z - b.z) <= tolerance);
};

ms.vec3.prototype.coordinatesClose = function(b, tolerance) {
	return ms.vec3.coordinatesClose(this, b, tolerance);
};

ms.vec3.unitVec = function(angle) {
	return new ms.vec3(Math.cos(angle), Math.sin(angle), 0);
};

/*
ms.vec3.angle = function(startPos, endPos) {
	var dx = endPos.x - startPos.x;
	var dy = endPos.y - startPos.y;
	return Math.atan2(dy, dx);
}; */

ms.vec3.prototype.move = function(dx, dy, dz) {
	this.x += dx;
	this.y += dy;
	this.z += dz;
};

// Convert from a vector to a matrix.
ms.vec3.prototype.toMatrix = function() {
	return mathG.matrix([[this.x], [this.y], [this.z]]);
};

// Return two vectors orthonormal to this one.
ms.vec3.prototype.orthonormal = function() {
	var q = new ms.vec3(1, 0, 0);
	if (Math.abs(this.dot(q)) > 0.9) {
		q = new ms.vec3(0, 1, 0);
	}
	var v = this.cross(q);
	var u = v.cross(this);
	u.normalize();
	v.normalize();
	return {u, v};
};

// The angle axis required to rotate one vector to parallel to the other one.
ms.vec3.angleAxis = function(p, q) {
	var axis = p.cross(q);
	if (Math.abs(axis.length2) < 1e-6) {
		return {axis: new ms.vec3(0, 0, 1), angle: 0};
	}
	axis.normalize();
	// The p, ay, and axis would be x, y, and z if p and q were on the z = 0 plane.
	ay = axis.cross(p);
	ay.normalize();

	var angle = Math.atan2(ay.dot(q), p.dot(q));
	return {axis, angle: 180 / Math.PI * angle};
};

// Swap axes going from Y is up to Z is up.
ms.vec3.prototype.swapAxes = function() {
	return new ms.vec3(this.z, this.x, this.y);
};

ms.vec3.prototype.export = function() {
	return { x: this.x, y: this.y, z: this.z };
};

ms.vec3.EPS = 1e-5;

ms.vec3.import = function(json) {
	var result = new ms.vec3(json.x, json.y, json.z);
	// Round very small numbers to 0. This fixes an issue with computing the turns.
	// TODO: Make computing turns more robust.
	for (var i = 0; i < 3; i++) {
		if (Math.abs(result.getValue(i)) < ms.vec3.EPS) {
			result.setValue(0, i);
		}
	}
	result.normalize();
	return result;
};