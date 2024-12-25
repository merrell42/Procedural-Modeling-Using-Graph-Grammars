// Interior is a face. Boundary is an edge.
ms.primalFace = function(type, turns, opt_wildcard) {
	this.boundary = [];
	this.interior = [];
	this.type = type;
	this.turns = turns;
	// If the turns can have any value. Used in splicing.
	this.wildcard = opt_wildcard || false;

	this.id = ms.counter.add('primalFace');
};

ms.primalFace.prototype.getType     = function(){	return this.type; };
ms.primalFace.prototype.getBoundary = function(){	return this.boundary[0]; };
ms.primalFace.prototype.getInterior = function(){	return this.interior[0]; };
ms.primalFace.prototype.getTurns    = function(){	return this.turns; };

ms.primalFace.prototype.setTurns = function(turns){
	this.turns = turns;
};

ms.primalFace.prototype.setWildcard = function(wildcard){
	this.wildcard = wildcard;
};

ms.primalFace.prototype.getWildcard = function(turns){
	return this.wildcard;
};

ms.primalFace.prototype.copy = function(){
	return new ms.primalFace(this.type, this.turns, this.wildcard);
};

ms.primalFace.prototype.export = function(){
	var i = this.getInterior();
	var b = this.getBoundary();
	return {
		interior: i ? i.getNetwork().faceIndex(this.getInterior()) : -1,
		boundary: b ? b.getNetwork().edgeIndex(this.getBoundary()) : -1,
		type: i.getNetwork().getBoundNet().getTypes().faceTypes.indexOf(this.type),
		turns: this.turns,
		wildcard: this.wildcard,
	};
};

ms.primalFace.prototype.import = function(boundNet, types, json){
	var interiorB = json.interior >= 0 ? boundNet.interior.getFaces()[json.interior] : null;
	var boundaryB = json.boundary >= 0 ? boundNet.boundary.getEdges()[json.boundary] : null;
	ms.boundNet.connectBoundary(this, boundaryB);
	ms.boundNet.connectInterior(this, interiorB);
	this.type = types.faceTypes[json.type];
	this.turns = json.turns;
	this.wildcard = json.wildcard;
};

ms.primalFace.prototype.print = function() {
	console.log(this.boundaryString());
	this.getInterior().print();
};

ms.primalFace.prototype.computeTurns = function() {
	var halfEdge = this.getInterior().getOuterComponent();
	if (!halfEdge) {
		this.turns = 0;
		return;
	}
	var faceHalfs = ms.faceNet.getConnectedHalfEdges(halfEdge);
	var looped = (faceHalfs[faceHalfs.length - 1].getNext() == faceHalfs[0]);
	if (!looped) {
		faceHalfs.pop();
	}
	var angles = faceHalfs.map((halfEdge) => { return halfEdge.getAngle(); });
	// These halfEdges turn clockwise around the face. Reverse it to go counter-clockwise.
	angles.reverse();
	var oppositeAngle = (angle) => ms.util.fixAngle(angle + Math.PI);
	angles = angles.map(oppositeAngle);
	if (!looped) {
		angles.unshift(oppositeAngle(angles[0]));
	} else {
		angles.push(angles[0]);
	}

	this.turns = ms.util.wedgeTurns(angles);
};

ms.primalFace.prototype.boundaryString = function() {
	var result = '';
	for (var i = 0; i < this.turns; i++) {
		result += '^';
	}
	for (var i = 0; i > this.turns; i--) {
		result += 'v';
	}
	var halfEdge = this.getInterior().getOuterComponent();
	if (halfEdge) {
		result += halfEdge.boundaryString();
	}
	return result;
};

ms.counter.register('primalFace');