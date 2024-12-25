ms.bspPolygon = function(points, face) {
	var stats = face.getNode().getStats();
	var properties = {
		bspNode: new ms.singleProperty('bspNode', false),
		face: new ms.singleProperty('face', true),
	};
	this.node = new ms.node(this, stats, 'bspPolygon', properties);
	this.points = points;

	face.getNode().connect(this);
};

ms.bspPolygon.prototype.getNode = function() {
	return this.node;
};

ms.bspPolygon.prototype.getFace = function() {
	return this.node.get('face')
};

ms.bspPolygon.create = function(face) {
	var points = face.getEndpoints().map(function(endpoint) {
		return endpoint.getPosition();
	});
	return new ms.bspPolygon(points, face);
};

ms.bspPolygon.prototype.getPoints = function() {
	return this.points;
};

ms.bspPolygon.prototype.selfIntersects = function() {
	for (var i = 0; i < this.points.length - 1; i++) {
		var a0 = this.points[i];
		var a1 = this.points[i + 1];
		for (var j = i + 1; j < this.points.length - 1; j++) {			
			var b0 = this.points[j];
			var b1 = this.points[j + 1];
			if (ms.intersector.intersect3D(a0, a1, b0, b1)) {
				return true;
			}
		}
	}
	return false;
};

ms.bspPolygon.prototype.print = function() {
	this.getFace().print();
};
