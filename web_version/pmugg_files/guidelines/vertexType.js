ms.vertexType = function(decoration) {
	this.connections = [];
	this.decoration = decoration;
	this.spliced = false;

	this.id = ms.counter.add('vertexType');
};

ms.counter.register('vertexType');

ms.vertexType.prototype.export = function(types) {
	var connections = this.connections.map((c) => ({
		adjustedAngle: c.adjustedAngle,
		angle: c.angle,
		dir: c.dir && c.dir.export(),
		directedId: c.directedId,
		edge: types.edgeTypes.indexOf(c.edge),
		faceIds: c.faceIds,
		isAtStart: c.isAtStart,
	}));
	return {
		connections,
		decoration: this.decoration && this.decoration.export(),
		spliced: this.spliced,
	};
};

ms.vertexType.import = function(json, types) {
	var decoration = json.decoration && ms.vertexDecoration.import(json.decoration, () => {});
	var result = new ms.vertexType(decoration);
	result.connections = json.connections.map((c) => ({
		adjustedAngle: c.adjustedAngle,
		angle: c.angle,
		dir: c.dir && ms.vec3.import(c.dir),
		directedId: c.directedId,
		edge: types.edgeTypes[c.edge],
		faceIds: c.faceIds,
		isAtStart: c.isAtStart,
	}));
	result.spliced = json.spliced;
	return result;
};

ms.vertexType.partialImport = function(json, edgeTypes) {
	var result = new ms.vertexType(new ms.vertexDecoration(() => {}));
	result.connections = json.connections.map((c) => {
		var edge = edgeTypes[c.edge];
		var faceIds;
		if (c.faceIds && c.faceIds.length) {
			faceIds = c.faceIds;
		} else {
			faceIds = edge.faceData.map((datum) => (datum.type.id));
		}
		var dir = edge.dir.copy();
		if (!c.isAtStart) {
			dir.scale(-1);
		}
		return {
			dir,
			edge,
			faceIds,
			isAtStart: c.isAtStart,
		}
	});
	result.id = json.id;
	return result;
};

ms.vertexType.prototype.getConnections = function() {
	return this.connections;
};

ms.vertexType.prototype.getDecoration = function() {
	return this.decoration;
};

ms.vertexType.prototype.setDecoration = function(decoration) {	
	this.decoration = decoration;
};

ms.vertexType.prototype.getSpliced = function() {
	return this.spliced;
};

ms.vertexType.prototype.setSpliced = function(spliced) {
	this.spliced = spliced;
};

ms.vertexType.adjustedAngle = function(angle, edge, isAtStart) {
	var directedId = 2 * edge.id + (isAtStart ? 0 : 1);
	return angle + 1e-5 * directedId;
};

ms.vertexType.prototype.addEdge = function(edge, isAtStart, angle, faceIds) {
	// Angle becomes the angle going out from the vertex through the edge.
	var dir = edge.getDir().copy();
	if (!isAtStart) {
		angle += Math.PI;
		dir.scale(-1);
	}
	angle = ms.util.fixAngle(angle);
	var index = 0;
	
	var directedId = 2 * edge.id + (isAtStart ? 0 : 1);
	var conns = this.connections;
	
	// The adjusted angle acts as a tie breaker when the angles are equal.
	var adjustedAngle = ms.vertexType.adjustedAngle(angle, edge, isAtStart);
	while (index < conns.length &&
		((conns[index].adjustedAngle < adjustedAngle) ||
		 (conns[index].adjustedAngle == adjustedAngle && conns[index].directedId < directedId))) {
		index++;
	}
	this.connections.splice(index, 0, {edge, isAtStart: isAtStart, angle, adjustedAngle, dir, directedId, faceIds});
	return index;
};

ms.vertexType.compare = function(endpointA, endpointB) {
	var angleA = endpointA.getAngle();
	var angleB = endpointB.getAngle();
	var idA = 2 * endpointA.getEdge().getCore().getId() + endpointA.getEdgeIndex();
	var idB = 2 * endpointB.getEdge().getCore().getId() + endpointB.getEdgeIndex();
	return (angleA > angleB) || ((angleA == angleB) && (idA > idB));
};

ms.vertexType.prototype.compare = function(endpointA, endpointB) {
	return ms.vertexType.compare(endpointA, endpointB);
};

ms.vertexType.prototype.computeFaceIds = function() {
	var N = this.connections.length
	for (var i = 0; i < N; i++) {
		var faceIds = [i, (i + 1) % N];
		if (!this.connections[i].isAtStart) {
			faceIds.reverse()
		}
		this.connections[i].faceIds = faceIds;
	}
};
