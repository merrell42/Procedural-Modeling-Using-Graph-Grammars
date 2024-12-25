// Interior is an vertex. Boundary should be null.
ms.primalEdge = function(type) {
	this.boundary = [];
	this.interior = [];
	this.type = type;

	this.id = ms.counter.add('primalEdge');
};

ms.primalEdge.prototype.getType     = function(){	return this.type; };
ms.primalEdge.prototype.getBoundary = function(){	return this.boundary[0]; };
ms.primalEdge.prototype.getInterior = function(){	return this.interior[0]; };

ms.primalEdge.prototype.export = function() {
	var i = this.getInterior();
	var type = i.getNetwork().getBoundNet().getTypes().edgeTypes.indexOf(this.type);
	return {
		interior: i ? i.getNetwork().edgeIndex(this.getInterior()) : -1,
		type,
	};
};

ms.primalEdge.prototype.import = function(boundNet, types, json){
	var interiorB = json.interior >= 0 ? boundNet.interior.getEdges()[json.interior] : null;
	ms.boundNet.connectInterior(this, interiorB);
	// var boundaryB = json.boundary >= 0 ? boundNet.boundary.getEdges()[json.boundary] : null;
	// ms.boundNet.connectBoundary(this, boundaryB);
	this.type = types.edgeTypes[json.type];
};

ms.primalEdge.prototype.copy = function(){
	return new ms.primalEdge(this.type);
};

ms.primalEdge.prototype.boundaryString = function() {
	return this.type.boundaryString();
};

ms.primalEdge.prototype.print = function() {
	this.interior.print();
};

ms.counter.register('primalEdge');