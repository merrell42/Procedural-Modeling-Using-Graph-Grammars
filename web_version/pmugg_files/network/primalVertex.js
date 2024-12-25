// Interior is a vertex. Boundary is a face or edge.
ms.primalVertex = function(type, opt_connection) {
	this.boundary = [];
	this.interior = [];
	this.type = type;

	// This is just for fixing the order with partial imports.
	this.connection = opt_connection;

	this.id = ms.counter.add('primalVertex');
};

ms.counter.register('primalVertex');

ms.primalVertex.prototype.getType     = function(){	return this.type; };
ms.primalVertex.prototype.getBoundary = function(){	return this.boundary[0]; };
ms.primalVertex.prototype.getInterior = function(){	return this.interior[0]; };

ms.primalVertex.prototype.copy = function(){
	return new ms.primalVertex(this.type);
};


ms.primalVertex.prototype.print = function() {
	this.getInterior().print();
};

ms.primalVertex.prototype.import = function(boundNet, types, json){
	var interiorB = json.interior >= 0 ? boundNet.interior.getVertices()[json.interior] : null;
	var boundaryB = json.boundary >= 0 ? boundNet.boundary.getVertices()[json.boundary] : null;
	ms.boundNet.connectInterior(this, interiorB);
	ms.boundNet.connectBoundary(this, boundaryB);
	if (json.kind == 'v') {
		this.type = types.vertexTypes[json.type];
	} else if (json.kind == 'e') {
		this.type = types.edgeTypes[json.type];
	}
};

ms.primalVertex.prototype.getBoundNet = function() {
	return this.getInterior().getNetwork().getBoundNet();
};

ms.primalVertex.prototype.connectorIndex = function() {
	return this.getBoundNet().getConnectors().indexOf(this);
};

ms.primalVertex.prototype.interiorEdge = function() {
	var halfs = this.getInterior().getHalfEdges();
	return halfs.find((half) => { return half.getEdge() }).getEdge();
};

ms.primalVertex.prototype.requiresShapeView = function () {
	return true;
};
