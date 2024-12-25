// Boundary is a face. Interior should be null.
ms.primalVolume = function(type) {
	this.boundary = [];
	this.interior = [];
	this.type = type;

	this.id = ms.counter.add('primalVolume');
};

ms.primalVolume.prototype.getType     = function(){	return this.type; };
ms.primalVolume.prototype.getBoundary = function(){	return this.boundary[0]; };
ms.primalVolume.prototype.getInterior = function(){	return this.interior[0]; };

// Turns don't matter for volumes.
ms.primalVolume.prototype.getTurns = function(){	return 0; };
ms.primalVolume.prototype.setTurns = function(turns){ };

ms.primalVolume.prototype.copy = function(){
	return new ms.primalVolume(this.type);
};

ms.primalVolume.prototype.export = function(){
	var i = this.getInterior();
	var b = this.getBoundary();
	return {
		boundary: b ? b.getNetwork().faceIndex(b) : -1,
		type: this.type,
	};
};

ms.primalVolume.prototype.import = function(boundNet, types, json){
	var boundaryB = json.boundary >= 0 ? boundNet.boundary.getFaces()[json.boundary] : null;
	ms.boundNet.connectBoundary(this, boundaryB);
	// var interiorB = json.interior >= 0 ? boundNet.interior.getFaces()[json.interior] : null;
	// ms.boundNet.connectInterior(this, interiorB);
	this.type = json.type;
};

ms.primalVolume.prototype.boundaryString = function() {
	var outerHalfs = this.getBoundary().getOuterHalfEdges();
	var result = '';
	outerHalfs.forEach((half) => {
		result += half.getEdge().getPrimal().boundaryString();
	});
	return result;
};

ms.primalVolume.prototype.print = function() {
	console.log(this.boundaryString());
	this.boundary.print();
};

ms.counter.register('primalVolume');