ms.endpoint = function(stats, isAtStart, edgeType, angle, dir, scale, createFace, faceIndex) {
	this.isAtStart = isAtStart;
	this.edgeType = edgeType;
	this.dir = dir; // Maybe this should be a valueProperty like angle.
	this.scale = scale; // This is only needed for extending the line.
	this.faceIndex = faceIndex;
	var properties = {
			face: new ms.singleProperty('face'),
			line: new ms.singleProperty('line', /* required */ 'true'),
			vertex: new ms.singleProperty('vertex'),
			vertexState: new ms.singleProperty('vertexState'),
			angle: new ms.valueProperty('angle'),
			// All the face connections. There is only one face connection that is right bounded by
			// this endpoint. It appears first if one exists. The remaining connects are sorted
			// going from top to bottom.
			faceConnection: new ms.alternativeArray('faceConnection', /* required */ false),
	};
	this.node = new ms.node(this, stats, 'endpoint', properties);
	this.node.setValue('angle', angle);
	
	this.faceTypeCached = null;
	this.nextOnFaceCached = {};

	if (createFace) {
		var faceType = ms.globalSettings.get('Use Network') ? this.faceType() : null;
		var face = new ms.face(faceType, stats);
		face.createGroup();
		face.getNode().connect(this);
	}
};

// The deviation from the ideal that is imposed as a fraction of the total length.
ms.endpoint.DEVIATION = 0.025;

ms.endpoint.prototype.getNode = function() {
	return this.node;
};

ms.endpoint.prototype.isRigid = function() {
	return this.edgeType.getIsRigid();
};

ms.endpoint.prototype.getLine = function() {
	return this.node.get('line');
};

ms.endpoint.prototype.getFace = function() {
	return this.node.get('face');
};

ms.endpoint.prototype.getAngle = function() {
	return this.node.get('angle');
};

ms.endpoint.prototype.setAngle = function(angle) {
	this.node.setValue('angle', angle);
};

ms.endpoint.prototype.getDir = function() {
	return this.dir;
};

ms.endpoint.prototype.getConnections = function() {
	return this.node.get('faceConnection');
};

/* ms.endpoint.prototype.isBefore = function(endpoint) {
	return ms.vertexType.compare(endpoint, this);
}; */

ms.endpoint.prototype.getIsAtStart = function() {
	return this.isAtStart;
	// return this.getLine().getEndpoints()[0] == this;
};

ms.endpoint.prototype.getEdgeType = function() {
	return this.edgeType;
	//return this.getLine().getEdgeType();
};

ms.endpoint.prototype.getVertex = function() {
	return this.node.get('vertex');
};

ms.endpoint.prototype.getVertexState = function() {
	return this.node.get('vertexState');
};

ms.endpoint.prototype.isConflicted = function() {
	return !this.getTwin();
};

ms.endpoint.prototype.getPosition = function() {
	return this.getVertex().getPosition();
};

ms.endpoint.prototype.getLineState = function() {
	return this.getLine() ? this.getLine().getState(this.isAtStart) : null;
};

ms.endpoint.prototype.getSegment = function() {
	return this.getLine().getSegment(this.isAtStart);
};

ms.endpoint.prototype.move = function(newPosition) {
	ms.timerG.start('endpoint move');
	if (ms.globalSettings.get('Full Move')) {
		this.getSegment().setPosition(newPosition, this.isAtStart); 
	} else {
		this.getLineState().setPosition(newPosition, this.isAtStart);
	}
	ms.timerG.stop('endpoint move');
};

ms.endpoint.prototype.getTwin = function() {
	var line = this.getLine();
	if (line) {
		var endpoints = line.getEndpoints();
		var index = endpoints.indexOf(this);
		return endpoints[1 - index];
	} else {
		return null;
	}
};

ms.endpoint.prototype.desiredLength = function() {
	return this.scale * this.edgeType.getEdgeLength();
};

ms.endpoint.prototype.idealOffset = function() {
	var offset = ms.vec2.unitVec(this.getAngle());
	offset.scale(this.desiredLength());
	return offset;
};

ms.endpoint.prototype.randomOffset = function() {
	var u = this.idealOffset();
	var deviationAmount = ms.endpoint.DEVIATION * this.desiredLength();
	var deviation = new ms.vec2(deviationAmount * ms.randomGaussian(), deviationAmount * ms.randomGaussian());
	u.add(deviation);
	return u;
};

ms.endpoint.prototype.faceType = function() {
	if (!this.faceTypeCached) {
		var faceData = this.edgeType.faceData;
		if (ms.globalSettings.get('Use Network')) {
			return faceData[this.faceIndex].type;
		} else {
			ms.alert('This should only happen for networks.');
		}
	}
	return this.faceTypeCached;
};


ms.endpoint.prototype.next3D = function() {
	return this.nextOnFace(this.faceType());
};

// Get the next endpoint while traversing the face in the counter-clockwise direction.
// The endpoints here act as half edges in a doubly connected edge list (DCEL).
ms.endpoint.prototype.next = function() {
	if (ms.globalSettings.get('Use Network')) {
		var endpoints = this.getFace().getEndpoints();
		var N = endpoints.length;
		var index = endpoints.indexOf(this);
		return endpoints[(index + 1) % N];
	}

	if (this.edgeType.is3D()) {
		return this.next3D();
	}
	var twin = this.getTwin();
	return twin && twin.clockwise();
};

// Get the previous endpoint.
ms.endpoint.prototype.prev = function() {
	if (ms.globalSettings.get('Use Network')) {
		var endpoints = this.getFace().getEndpoints();
		var index = endpoints.indexOf(this);
		if (index == 0) {
			var N = endpoints.length;
			index = N;
		}
		return endpoints[index - 1];
	}

	var counter = this.counter();
	return counter && counter.getTwin();
};

ms.endpoint.prototype.clockwise = function() {
	var vertexState = this.getVertexState();
	if (!vertexState) {
		return null;
	}
	var endpoints = vertexState.getEndpoints();
	var index = endpoints.indexOf(this);
	if (index == -1) {
		ms.alert('Vertex is missing endpoint in clockwise.');
	}
	var nextIndex = index > 0 ? index - 1 : endpoints.length - 1;	
	return endpoints[nextIndex];
};

// Get the previous endpoint.
ms.endpoint.prototype.counter = function() {
	var vertexState = this.getVertexState();
	if (!vertexState) {
		return null;
	}
	var endpoints = vertexState.getEndpoints();
	var index = endpoints.indexOf(this);
	if (index == -1) {
		ms.alert('Vertex is missing endpoint in next.');
	}
	var prevIndex = (index + 1) % endpoints.length;
	return endpoints[prevIndex];
};

ms.endpoint.prototype.nextFace = function() {
	var lineStates = this.getVertex().getLineStates();
	if (lineStates[0]) {
		return lineStates[0].getLine().getEndpoints()[0].getFace();
	} else {
		this.next().print();
	}
};


ms.endpoint.prototype.angleOffset = function() {
	var defaultAngle = this.getLine().getEdgeType().getAngle() + (this.getIsAtStart() ? 0 : Math.PI);
	return ms.util.fixAngle(this.getAngle() - defaultAngle);
};

ms.endpoint.prototype.mergeFaces = function(next) {
	this.getFace().append(next.getFace());
};

ms.endpoint.prototype.maybeMergeNextFace = function() {
	var next = this.next();
	if (next) {
		this.getFace().append(next.getFace());
	}
};

ms.endpoint.prototype.maybeMergePrevFace = function() {
	var prev = this.prev();
	if (prev) {
		prev.getFace().append(this.getFace());
	}
};

ms.endpoint.prototype.transfer = function(replacement) {
	var index = this.getLine().getEndpoints().indexOf(this);
	replacement.addEndpoint(this, index);
};

ms.endpoint.prototype.checkSupport = function() {
	if (this.node.isDestroyed()) {
		return;
	}
	if (!this.getVertexState() && (!this.getTwin() || !this.getTwin().getVertexState())) {
		this.getLine().destroy();
	}
};

// True if the first connection is right bound.
ms.endpoint.prototype.hasRightBound = function() {
	var connections = this.getConnections();
	return connections[0] && !connections[0].isLeft(this)
};

// Find the index at which to splice in the connection.
ms.endpoint.prototype.spliceIndex = function(connectionA) {
	var connections = this.getConnections();
	var index = 0;
	// Skip the right bound connection if it exists.
	if (this.hasRightBound) {
		index++;
	}
	var yA = connectionA.getCoordinates()[0].y;
	var yB = Infinity;
	while (yA < yB) {
		if (index >= connections.length) {
			return index;
		}
		yB = connections[index].getCoordinates()[0].y;
		index++;
	}
	// We went past the right index. Backtrack by 1.
	return index - 1;
};

ms.endpoint.prototype.drawFace = function(drawPointFunc) {
	var segments = this.getLine().getSegments();
	for (var j = 0; j < segments.length; j++) {
		var states = segments[j].getStates();
		for (var i = 0; i < states.length; i++) {
			var state = states[this.isAtStart ? i : states.length - 1 - i];
			var position = state.getCoordinates().getAngledEdges()[this.isAtStart ? 0 : 1].getPosition();
			drawPointFunc(position);
		}
	}
};

ms.endpoint.prototype.fillRenderPositions = function(positions, dims) {
	if (dims == 2) {
		var segments = this.getLine().getSegments();
		for (var j = 0; j < segments.length; j++) {
			var states = segments[j].getStates();
			for (var i = 0; i < states.length; i++) {
				var state = states[this.isAtStart ? i : states.length - 1 - i];
				var position = state.getCoordinates().getAngledEdges()[this.isAtStart ? 0 : 1].getPosition();
				positions.push(position.x);
				positions.push(position.y);
			}
		}
	} else {
		var scale = ms.view3d.GENERATED_SCALE;
		var position = this.getVertex().getPosition();
		positions.push(scale * position.x);
		positions.push(scale * position.y);
		positions.push(scale * position.z);
	}
};

// Returns true if the endpoint is oriented correctly around the face. That is the
// face is on the left side of the endpoint and the endpoint is winding counter-
// clockwise around the face.
// Should match ms.graphEndpoint.oriented.
ms.endpoint.prototype.oriented = function(faceType) {
	var faceData = this.edgeType.getFaceData();
	var faceDatum = faceData.find(function(datum) { return datum.type == faceType; });
	if (!faceDatum) {
		ms.alert('Face type is not found on endpoint\'s edge ');
		return false;
	}
	var edgeIndex = this.isAtStart ? 0 : 1;
	return !!(faceDatum.onRight ^ edgeIndex)
};

// Should match ms.graphEndpoint.nextOnFace.
ms.endpoint.prototype.nextOnFace = function(faceType) {
	var id = faceType.id;
	if (!this.nextOnFaceCached[id]) {
		var twin = this.getTwin();
		var angle0 = faceType.angle(twin.getDir());
		var nextVertex = twin.getVertex();
		// Find all endpoints with the same face type.
		var endpoints = nextVertex.getEndpoints();
		// if (nextVertex.isOuter()) { return null; }
		endpoints = endpoints.filter(function(endpoint) {
			if (endpoint == twin) {
				return false;
			}
			return endpoint.edgeType.getFaceData().find(function(faceDatum) {
				return faceDatum.type == faceType;
			});
		});
		// If there is only one endpoint, pick it.
		if (endpoints.length == 1) {
			this.nextOnFaceCached[id] = endpoints[0];
		} else {
			var angles = endpoints.map(function(endpoint) {
				return faceType.angle(endpoint.getDir());
			});
			// Find the maximum angle that is less than angle0.
			for (var i = 0; i < angles.length; i++) {
				if (angles[i] >= angle0) {
					angles[i] -= 2 * Math.PI;
				}
			}
			var maxAngle = angles[0]
			var maxIndex = 0;
			for (var i = 1; i < angles.length; i++) {
				if (angles[i] > maxAngle) {
					maxAngle = angles[i];
					maxIndex = i;
				}
			}
			this.nextOnFaceCached[id] = endpoints[maxIndex];
		}
	}
	return this.nextOnFaceCached[id];
};

ms.endpoint.lineThickness = 0.1;

ms.endpoint.drawLine = function(p0, p1, color, renderData) {
	var colorData = renderData.getColorData(color);

	var u = p1.copy().minus(p0);
	var v = new ms.vec2(u.y, -u.x);
	v.normalize().scale(ms.endpoint.lineThickness / 2);
	var p00 = p0.copy().minus(v);
	var p01 = p0.copy().add(v);
	var p10 = p1.copy().minus(v);
	var p11 = p1.copy().add(v);
	colorData.positions.push(p00.x);
	colorData.positions.push(p00.y);
	colorData.positions.push(p01.x);
	colorData.positions.push(p01.y);
	colorData.positions.push(p10.x);
	colorData.positions.push(p10.y);
	colorData.positions.push(p01.x);
	colorData.positions.push(p01.y);
	colorData.positions.push(p10.x);
	colorData.positions.push(p10.y);
	colorData.positions.push(p11.x);
	colorData.positions.push(p11.y);
};

ms.endpoint.prototype.fillHighlight = function(renderData) {
	var u = ms.vec2.unitVec(this.getAngle());
	var v = new ms.vec2(u.y, -u.x);
	var p1 = new ms.vec2();
	var p2 = new ms.vec2();
	var p3 = new ms.vec2();
	var str = 0.5;
	var diag = 0.2;
	var center = this.getVertex().getPosition();
	p1.x = center.x + str * u.x;
	p1.y = center.y + str * u.y;
	p2.x = center.x + str * u.x - diag * u.x - diag * v.x;
	p2.y = center.y + str * u.y - diag * u.y - diag * v.y;
	p3.x = center.x + str * u.x - diag * u.x + diag * v.x;
	p3.y = center.y + str * u.y - diag * u.y + diag * v.y;

	ms.endpoint.drawLine(center, p1, '#004444', renderData);
	ms.endpoint.drawLine(p1, p2, '#004444', renderData);
	ms.endpoint.drawLine(p1, p3, '#004444', renderData);
};

ms.endpoint.drawArrow = function(context, center, angle, color) {
	var u = ms.vec2.unitVec(angle);
	var v = new ms.vec2(u.y, -u.x);
	var p1 = new ms.vec2();
	var p2 = new ms.vec2();
	var p3 = new ms.vec2();
	var str = 15;
	var diag = 5;
	p1.x = center.x + str * u.x;
	p1.y = center.y - str * u.y;  // The y-values are backwards.
	p2.x = center.x + str * u.x - diag * u.x - diag * v.x;
	p2.y = center.y - str * u.y + diag * u.y + diag * v.y;
	p3.x = center.x + str * u.x - diag * u.x + diag * v.x;
	p3.y = center.y - str * u.y + diag * u.y - diag * v.y;
		
	context.strokeStyle = color;
	context.lineWidth = 2;
	context.beginPath();
	context.moveTo(center.x, center.y);
	context.lineTo(p1.x, p1.y);
	context.stroke();
	context.beginPath();
	context.moveTo(p2.x, p2.y);
	context.lineTo(p1.x, p1.y);
	context.stroke();
	context.beginPath();
	context.moveTo(p3.x, p3.y);
	context.lineTo(p1.x, p1.y);
	context.stroke();
};

ms.endpoint.prototype.draw3D = function(view, convertToScreen, highlighted) {
	var scale = ms.view3d.GENERATED_SCALE;
	var start = this.getVertex().getPosition().copy().scale(scale);
	var end = this.next().getVertex().getPosition().copy().scale(scale);
	view.drawArrow(start, end, '#f0f');
};

ms.endpoint.prototype.draw = function(context, convertToScreen, highlighted) {
	if (context instanceof ms.view3d) {
		return this.draw3D(context, convertToScreen, highlighted);
	}
	var vertex = this.getVertex();
	if (vertex) {
		var color = highlighted ? '#044' : '#484'
		ms.endpoint.drawArrow(context, convertToScreen(vertex.getPosition()), this.getAngle(), color);
	}
};

ms.endpoint.prototype.highlight = function(context, convertToScreen) {
	this.draw(context, convertToScreen, true);
};


ms.endpoint.prototype.print = function() {
	var angleStr = (this.getAngle() * 180 / Math.PI).toFixed(2);
	if (this.getVertex()) {
		window.console.log(angleStr + ' ' + this.getVertex().getPosition().print());
	} else {
		window.console.log(angleStr + ' ' + 'No vertex');
	}
	ms.highlight(this);
};
