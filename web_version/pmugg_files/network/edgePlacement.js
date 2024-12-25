ms.edgePlacement = function(edge, id, settings) {
	this.edge = edge;
	this.id = id;
	this.vertexIds = edge.getEndpoints().map((endpoint) => {
		return endpoint.getVertex().getId();
	});
	if (this.vertexIds.length == 1) {
		var endpoint = edge.getEndpoints()[0].next();
		this.vertexIds.push(endpoint.getVertex().getId());
	}
	this.dir = edge.getEndpoints()[0].getDir();
	this.settings = settings;
	// The number of vertices that are fully constrained.
	this.constraints = [];
};

ms.edgePlacement.prototype.initialize = function() {
	var { settings } = this;
	var faces0 = settings.getVertex(this.vertexIds[0]).freeFaceIds;
	var faces1 = settings.getVertex(this.vertexIds[1]).freeFaceIds;

	var intersection = faces0.filter((id) => {
		return faces1.includes(id);
	});

	var faceIds = [intersection[0]];
	var fPlace0 = this.settings.getFace(intersection[0]);
	for (var i = 1; i < intersection.length; i++) {
		var fPlaceI = this.settings.getFace(intersection[i]);
		if (!fPlace0.coplanar(fPlaceI)) {
			faceIds.push(intersection[i]);
		}
	}
	if (faceIds.length < 2) {
		var normal = fPlace0.getNormal().cross(this.dir);
		var id = this.settings.createFace(normal);

		settings.getVertex(this.vertexIds[0]).addFreeFace(id);
		settings.getVertex(this.vertexIds[1]).addFreeFace(id);
	}
};

ms.edgePlacement.prototype.addConstraint = function(id) {
	if (!this.constraints.includes(id)) {
		this.constraints.push(id);
	}
	if (this.constraints.length == 2) {
		this.settings.addToOrder(this.id, 'edge');
	}
};

ms.edgePlacement.prototype.getRange = function() {
	var vPlace0 = this.settings.getVertex(this.vertexIds[0]);
	var vPlace1 = this.settings.getVertex(this.vertexIds[1]);
	var mb0 = vPlace0.getChangeMB();
	var mb1 = vPlace1.getChangeMB();
	var mLength = this.dir.dot(mb1.m.copy().minus(mb0.m));
	var bLength = this.dir.dot(mb1.b.copy().minus(mb0.b));
		
	var brush = this.edge && this.edge.getEdgeType().getBrush();
	var lengthMin = brush ? brush.get('Min Length') : ms.netTransistor.defaultLengthMin;
	var lengthMax = brush ? brush.get('Max Length') : ms.netTransistor.defaultLengthMax;
	var tileLength = 0;
	if (brush && brush.get('Rigid Tiled')) {
		tileLength = brush.get('Tile Length');
	}

	return ms.range.transformCreate(mLength, bLength, new ms.range(lengthMin, lengthMax, tileLength));
};

ms.edgePlacement.prototype.print = function() {
	this.edge.print();
};
