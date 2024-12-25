ms.faceType3D = function(material, normal) {
	this.material = material;
	this.normal = normal;
	var {u, v} = normal.orthonormal();
	this.u = u;
	this.v = v;
	this.monotonic = false;
	this.color = null;

	// Compute the maximum dimension of the normal.
	this.maxDim = ms.util.maxDim(normal);

	this.id = ms.faceType3D.count++;
};

ms.faceType3D.count = 0;

ms.faceType3D.EPS = 1e-5;

ms.faceType3D.prototype.export = function() {
	return {
		color: this.color,
		material: this.material,
		normal: this.normal.export(),
		// maxDim: this.maxDim,
		// u: this.u.export(),
		// v: this.v.export(),
	}
};

ms.faceType3D.import = function(json) {
	var result = new ms.faceType3D(json.material, ms.vec3.import(json.normal));
	result.color = json.color;
	return result;
};

ms.faceType3D.partialImport = function(json) {
	var normal = ms.vec3.import(json.normal).swapAxes();
	var result = new ms.faceType3D(null, normal);
	result.id = json.signature;
	result.idNum = json.id;
	return result;
};

ms.faceType3D.prototype.getMaterial = function() {
	return this.material;
};

ms.faceType3D.prototype.getNormal = function() {
	return this.normal;
};

ms.faceType3D.prototype.getMaxDim = function() {
	return this.maxDim;
};

// Find the angle within the face's coordinate system.
ms.faceType3D.prototype.angle = function(q) {
	var dx = this.u.dot(q);
	var dy = this.v.dot(q);
	var angle = Math.atan2(dy, dx);
	// Angles near pi wrap to -pi.
	if (angle > Math.PI - ms.faceType3D.EPS) {
		return -angle;
	} else {
		return angle;
	}
};

// Find the area of a face.
ms.faceType3D.prototype.getArea = function(model, face) {	
	var n = face.vertices.length;
	var vProjected = [];
	for (var i = 0; i < n; i++) {
		var vertex = model.vertices[face.vertices[i].vertexIndex];
		vProjected.push([this.u.dot(vertex), this.v.dot(vertex)]);
	}
	return -ms.shapeSet.polygonArea(vProjected);
};

ms.faceType3D.prototype.computeMonotonic = function(graphs) {
	var positive = false;
	var negative = false;
	var self = this;
	graphs.forEach(function(graph) {
		graph.getOuterEndpoints().forEach(function(endpoint) {
			if (endpoint.faceTypes().includes(self) && endpoint.oriented(self)) {
				var trace = ms.graph.traceToExit3D(endpoint, self);
				if (trace.winding > 0) {
					positive = true;
				}
				if (trace.winding < 0) {
					negative = true;
				}
			}
		});
	});
	this.monotonic = positive ^ negative;
};

ms.faceType3D.prototype.normalColor = function(normal) {
	if (this.color) {
		return this.color;
	}
	var normal = this.normal;
	// return [-0.35 * normal.x + 0.5, -0.3 * normal.y + 0.5, 0.2 * normal.z + 0.5, 1];
	return [0.5 * normal.x + 0.5, 0.5 * normal.y + 0.5, 0.5 * normal.z + 0.5, 1];
};
