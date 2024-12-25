ms.vertexState = function(stats, vertex, angle, scale, type) {
	this.angle = angle;
	this.scale = scale;
	this.type = type;
	
	var connections = this.type.getConnections();
	var properties = {
			endpoint: new ms.requiredArray('endpoint', /* required */ true, connections.length),
			ringInstance: new ms.alternativeArray('ringInstance', /* required */ false),
			vertex: new ms.singleProperty('vertex'),
			cost: new ms.valueProperty('cost', 'valence'),
	};
	this.node = new ms.node(this, stats, 'vertexState', properties);
	this.node.setValue('cost', this.type.getConnections().length - 2);
	this.node.connect(vertex);
};

ms.vertexState.prototype.getVertex = function() {
	return this.node.get('vertex');
};

ms.vertexState.prototype.getNode = function() {
	return this.node;
};

ms.vertexState.prototype.getType = function() {
	return this.type;
};

ms.vertexState.prototype.getEndpoints = function() {
	return this.node.get('endpoint');
};

ms.vertexState.prototype.getEndpoint = function(index) {
	return this.getEndpoints()[index];
};

ms.vertexState.prototype.removeLeaves = function(index) {
	var ringInstances = this.node.get('ringInstance').slice();
	for (var i = 0; i < ringInstances.length; i++) {
		this.node.disconnect(ringInstances[i]);
	}
};

ms.vertexState.prototype.resolveEndpoints = function() {
	if (ms.globalSettings.get('Use Network')) {
		this.resolveEndpointsNet();
	} else {
		this.resolveEndpointsGraph();
	}
};

ms.vertexState.prototype.resolveEndpointsNet = function() {
	var connections = this.type.getConnections();
	var vertexIndex = 0;
	for (var i = 0; i < connections.length; i++) {
		var connection = connections[i];
		var edgeType = connection.edge;
		edgeType.getFaceData().forEach((faceDatum, faceIndex) => {
			var position = faceDatum.onRight ^ connection.isAtStart;
			if (position) {
				this.createEndpointNet(connection, vertexIndex, faceIndex);
				vertexIndex++;
			}
		});
	}
}

ms.vertexState.prototype.createEndpointNet = function(connection, vertexIndex, faceIndex) {
	if (this.node.isDestroyed()) {
		ms.alert('Error in createEndpoint.');
		return null;
	}
	var dir = connection.dir.copy();
	if (this.angle != 0) {
		dir.rotate(this.angle);
	}
	var angle = ms.util.fixAngle(this.angle + connection.angle);
	var stats = this.node.getStats();
	var endpoint = new ms.endpoint(stats, connection.isAtStart, connection.edge, angle, dir, this.scale, true, faceIndex);
	
	var line = new ms.line(stats, connection.edge);
	var segment = new ms.lineSegment(stats);
	line.addSegments([segment]);
	line.addEndpoint(endpoint, faceIndex);
	this.addEndpoint(endpoint, vertexIndex);
	
	return endpoint;
};

ms.vertexState.prototype.resolveEndpointsGraph = function() {
	var newEndpoints = [];
	for (var i = 0; i < this.getEndpoints().length; i++) {
		if (!this.getEndpoint(i)) {
			var newEndpoint = this.createEndpointGraph(i);
			newEndpoint && newEndpoints.push(newEndpoint);
		}
	}
}

ms.vertexState.prototype.createEndpointGraph = function(index) {
	if (this.node.isDestroyed()) {
		ms.alert('Error in createEndpoint.');
		return null;
	}
	var connections = this.type.getConnections();
	var connection = connections[index];
	var dir = connection.dir.copy();
	if (this.angle != 0) {
		dir.rotate(this.angle);
	}
	var angle = ms.util.fixAngle(this.angle + connection.angle);
	var stats = this.node.getStats();
	var endpoint = new ms.endpoint(stats, connection.isAtStart, connection.edge, angle, dir, this.scale, true);
	
	var line = new ms.line(stats, connection.edge);
	var segment = new ms.lineSegment(stats);
	line.addSegments([segment]);
	line.addEndpoint(endpoint, connection.isAtStart ? 0 : 1);
	this.addEndpoint(endpoint, index);
	
	return endpoint;
};

ms.vertexState.prototype.endpointIndex = function(endpoint) {
	return this.getEndpoints().indexOf(endpoint);
};

ms.vertexState.prototype.addEndpoint = function(endpoint, index) {
	var oldState = endpoint.getVertexState();
	oldState && endpoint.getNode().disconnect(oldState)

	// This two insertions should be the same.
	// TODO: Simplify all this. The stuff with the endpoints and
	// vertex state is old overly complicated code.
	this.node.insert(endpoint, index);
	endpoint.getNode().connect(this.getVertex());
	endpoint.maybeMergePrevFace();

	if (this.getEndpoints().length > 1) {
		var twin = endpoint.getTwin();
		twin && twin.maybeMergeNextFace(twin);
	}
};

// Split the faces and destroy this state.
ms.vertexState.prototype.kill = function() {
	var endpoints = this.getEndpoints().slice();
	for (var i = 0; i < endpoints.length; i++) {
		if (endpoints[i]) {
			endpoints[i].getFace().split(endpoints[i]);
		}
	}
	this.node.destroy();
};

ms.vertexState.prototype.highlight = function(context, convertToScreen) {
	for (var i = 0; i < this.getEndpoints().length; i++) {
		var endpoint = this.getEndpoints()[i];
		endpoint && endpoint.highlight(context, convertToScreen);
	}
};

ms.vertexState.prototype.print = function() {
	var endpoints = this.getEndpoints();
	for (var i = 0; i < endpoints.length; i++) {
		window.console.log(endpoints[i].getAngle() * 180 / Math.PI);
	}
	window.console.log(this.getVertex().getPosition().print());
	ms.highlight(this);
};
