ms.bspPlane = function(n, d) {
	this.n = n;
	this.d = d;
};

ms.bspPlane.create = function(face) {
	var endpoint = face.getEndpoints()[0];
	var n = endpoint.faceType().getNormal();
	var d = n.dot(endpoint.getPosition());
	return new ms.bspPlane(n, d);
};

ms.bspPlane.parallelEps = 1e-3;
ms.bspPlane.coPlanarEps = 1e-5;

ms.bspPlane.prototype.getN = function() {
	return this.n;
};

ms.bspPlane.prototype.getD = function() {
	return this.d;
};

ms.bspPlane.prototype.isParallel = function(planeB) {
	return Math.abs(this.n.dot(planeB.getN())) > 1 - ms.bspPlane.parallelEps;
};

ms.bspPlane.prototype.sameD = function(planeB) {
	return Math.abs(this.d - planeB.d) < ms.bspPlane.coPlanarEps;
};

// Determine if a query point is above (+1), below (-1), or on the plane (0).
ms.bspPlane.prototype.sign = function(query) {
	var d = this.n.dot(query);
	if (d > this.d + ms.bspPlane.coPlanarEps) {
		return 1;
	} else if (d > this.d - ms.bspPlane.coPlanarEps) {
		return 0;
	} else {
		return -1;
	}
};

ms.bspPlane.prototype.crossingPoint = function(p0, p1) {
	var d0 = this.n.dot(p0);
	var d1 = this.n.dot(p1);
	var s = (this.d - d0) / (d1 - d0);
	return ms.vec3.lerp(p0, p1, s);
};