ms.line = function(stats, edgeType) {
	this.edgeType = edgeType;
	var numEndpoints = ms.globalSettings.get('Use Network') ? edgeType.getFaceData().length : 2;
	var properties = {
		lineSegment: new ms.requiredArray('lineSegment'),
		endpoint: new ms.requiredArray('endpoint', /* required */ false, numEndpoints),
		ringInstance: new ms.alternativeArray('ringInstance', /* required */ false),
		cost: new ms.valueProperty('cost', 'lineDistance'),
	};
	this.node = new ms.node(this, stats, 'line', properties);
	this.setCost(0);
	this.tileSeeds = {base: ms.random(100), increment: ms.random(100)};
};

ms.line.RENDER_WIDTH_3D = 0.5;
ms.line.backtrack = 0.1;

// TODO: Be more intelligent about this. Make lines that curve more have shorter state lengths.
ms.line.stateLength = 1;

ms.line.prototype.getNode = function() {
	return this.node;
};

ms.line.prototype.getSegments = function() {
	return this.node.get('lineSegment');
};

ms.line.prototype.getRingInstances = function() {
	return this.node.get('ringInstance');
};

ms.line.prototype.getSegment = function(isAtStart) {
	var segments = this.getSegments();
	return segments[isAtStart ? 0 : segments.length - 1];
};

ms.line.prototype.getState = function(isAtStart) {
	var states = this.getSegment(isAtStart).getStates();
	return states[isAtStart ? 0 : states.length - 1];
};

ms.line.prototype.getEdgeType = function() {
	return this.edgeType;
};

ms.line.prototype.getEndpoints = function() {
	return this.node.get('endpoint');
};

ms.line.prototype.getCost = function(cost) {
	return this.node.get('cost');
};

ms.line.prototype.setCost = function(cost) {
	this.node.setValue('cost', cost);
};

ms.line.prototype.getTileSeeds = function() {
	return this.tileSeeds;
};

ms.line.prototype.copy = function() {
	return new ms.line(this.node.getStats(), this.edgeType);
};

ms.line.prototype.addSegments = function(segments, opt_atStart) {
	var self = this;
	segments.slice().forEach(function(segment) {
		self.getNode().connect(segment, opt_atStart);
	});
};

ms.line.prototype.addEndpoint = function(endpoint, index) {
	this.node.insert(endpoint, index);
};

ms.line.prototype.isRigid = function() {
	var endpoints = this.getEndpoints();
	return (endpoints[0] && endpoints[0].isRigid() ||
			endpoints[1] && endpoints[1].isRigid());
};

// Split the line into two disconnected parts.
ms.line.prototype.split = function() {
	ms.timerG.start('split No Vertex');
	var stats = this.node.getStats();

	var newLines = [this.copy(), this.copy()];
	newLines[0].addSegments([new ms.lineSegment(stats)]);
	newLines[1].addSegments([new ms.lineSegment(stats)]);

	// Get old endpoints, set new ones.
	var endpoints = this.getEndpoints().slice();
	var nextEndpoints = endpoints.map((endpoint) => {
		return endpoint.next();
	});
	endpoints.forEach((endpoint) => {
		endpoint.getFace().split(endpoint.next());
	});
	endpoints.forEach((endpoint) => {
		endpoint.transfer(newLines[endpoint.getIsAtStart() ? 0 : 1]);
	});
	this.node.destroy();
	ms.timerG.stop('split No Vertex');
	return {lines: newLines, nextEndpoints};
};

ms.line.prototype.getBrush = function() {
	return this.edgeType.getBrush();
};

ms.line.prototype.fillFromEndpoints = function(addToModel) {
	var endpoints = this.getEndpoints();
	
	var endpoint0 = endpoints[0];
	var endpoint1 = endpoints[1] || endpoint0.next();
	var p0 = endpoint0.getPosition();
	var p1 = endpoint1.getPosition();
	var edges = [
		new ms.angledEdge(p0, endpoint0.getAngle(), 0),
		new ms.angledEdge(p1, endpoint1.getAngle(), 1)
	];
	var coordinates = new ms.lineStateCoordinates(edges, 1);
	var state = new ms.lineState(this.node.getStats(), coordinates);
	this.getSegment().addState(state, true);
	if (addToModel) {
		state.addToModel();
	}
};

// This remove the line from the cells, but does not add them back.
ms.line.prototype.moveToEndpoints = function() {
	var endpoints = this.getEndpoints();
	var segments = this.getSegments();
	if (segments.length == 1) {
		segments[0].setPositionsOneState([endpoints[0].getPosition(), endpoints[1].getPosition()]);
	} else {
		debugger;
		ms.alert('Moving two segments is untested.');
		// segments[0].setPositions(endpoints[0].getPosition(), true);
		// segments[1].setPositions(endpoints[1].getPosition(), false);
	}
};

ms.line.prototype.findIntersections = function() {
	var found = this.getSegments().some(function(segment) {
		return segment.findIntersections();
	});
	if (found) {
		this.setCost(1e6);
	}
	return found;
};

// I don't like this.
ms.line.prototype.getAngle = function() {
	return this.edgeType.getAngle();
};

ms.line.prototype.getDir = function() {
	return ms.vec2.unitVec(this.getAngle());
};

ms.line.prototype.getFaceData = function() {
	return [];
};

ms.line.createFromPositions = function(positions, angle0, edgeType, stats) {
	var angle1 = ms.util.fixAngle(angle0 + Math.PI);
	var dir0 = ms.vec2.unitVec(angle0);
	var dir1 = ms.vec2.unitVec(angle1);
	var endpoint0 = new ms.endpoint(stats, true, edgeType, angle0, dir0, 1, false);
	var endpoint1 = new ms.endpoint(stats, false, edgeType, angle1, dir1, 1, false);

	var vertex0 = new ms.vertex(stats, positions[0]);
	var vertex1 = new ms.vertex(stats, positions[1]);
	vertex0.getNode().connect(endpoint0);
	vertex1.getNode().connect(endpoint1);
	return ms.line.createFromEndpoints(stats, [endpoint0, endpoint1]);
};

ms.line.createFromEndpoints = function(stats, endpoints) {
	var edgeType = endpoints[0].getEdgeType();
	var line = new ms.line(stats, edgeType);
	for (var i = 0; i < 2; i++) {
		line.addEndpoint(endpoints[i], i);
	};
	line.addSegments([new ms.lineSegment(stats)]);
	line.fillFromEndpoints();
	return line;
};

ms.line.splitVertexTypes = {};

ms.line.getVertexType = function(edgeType, types) {
	if (!ms.line.splitVertexTypes[edgeType.id] ||
		ms.line.splitVertexTypes[edgeType.id].connections[0].edge !== edgeType) { // This means we are using an old version.
		var vertexType = new ms.vertexType();
		var faceIds = []; // Unsure about this.
		vertexType.addEdge(edgeType, true, edgeType.getAngle(), faceIds);
		vertexType.addEdge(edgeType, false, edgeType.getAngle(), faceIds);
		vertexType.setSpliced(true);
		ms.line.splitVertexTypes[edgeType.id] = vertexType;
		if (types) {
			types.vertexTypes.push(vertexType);
		}
	}
	return ms.line.splitVertexTypes[edgeType.id];
};

// Splitting lines is so complicated.
// Split line and add a vertex in between.
ms.line.prototype.fullSplit = function(s) {
	var middlePos = ms.vec3.lerp(
		this.getEndpoints()[0].getPosition(),
		this.getEndpoints()[1].getPosition(), s);
	var split = this.split();
	var vertexType = ms.line.getVertexType(this.getEdgeType());
	var newVertex = ms.vertex.createWithState(this.node.getStats(), middlePos, 0, 1, vertexType);
	var addedLines = newVertex.getEndpoints().map((endpoint) => endpoint.getLine());
	var addedFaces = newVertex.getEndpoints().map((endpoint) => endpoint.getFace());
	// Not sure why this is sometimes different. Some sort of parity.
	var p = !split.lines[0].getEndpoints()[0] ? 0 : 1;
	
	var start0 = newVertex.getEndpoints()[0].getIsAtStart();
	var endpointT = newVertex.getEndpoints()[start0 ? 0 : 1];
	var endpointF = newVertex.getEndpoints()[start0 ? 1 : 0];
	if (p == 0) {
		split.lines[0].addEndpoint(endpointF, 0);
		split.lines[1].addEndpoint(endpointT, 1);
		var prevEndpoint0 = split.lines[0].getEndpoints()[1];
		var prevEndpoint1 = split.lines[1].getEndpoints()[0];
		var startPrev0 = prevEndpoint0.getIsAtStart();
		prevEndpoint0.getFace().insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
		prevEndpoint1.getFace().insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
		prevEndpoint0.maybeMergeNextFace();
		prevEndpoint1.maybeMergeNextFace();
	} else {
		split.lines[0].addEndpoint(endpointF, 1);
		split.lines[1].addEndpoint(endpointT, 0);
		var prevEndpoint0 = split.lines[0].getEndpoints()[0];
		var prevEndpoint1 = split.lines[1].getEndpoints()[1];
		var startPrev0 = prevEndpoint0.getIsAtStart();
		prevEndpoint0.getFace().insert(startPrev0 ? endpointT : endpointF, prevEndpoint0);
		prevEndpoint1.getFace().insert(startPrev0 ? endpointF : endpointT, prevEndpoint1);
		prevEndpoint0.maybeMergeNextFace();
		prevEndpoint1.maybeMergeNextFace();
	}
	split.lines[0].fillFromEndpoints(true)
	split.lines[1].fillFromEndpoints(true)
	addedLines.forEach((line) => { line.getNode().destroy(); });
	addedFaces.forEach((face) => { face.getNode().destroy(); });
	return { split, newVertex };
};

// We assume this is only for 3D drawing. 2D drawing is handled by lineState, I think.
ms.line.prototype.draw = function(view, offset) {
	var endpoints = this.getEndpoints();
	if (endpoints[0] && endpoints[1]) {
		var start = endpoints[0].getPosition().copy().scale(ms.view3d.GENERATED_SCALE);
		var end = endpoints[1].getPosition().copy().scale(ms.view3d.GENERATED_SCALE);
		view.drawEdge(start, end, '#000', 0, ms.line.RENDER_WIDTH_3D);
	}
};

ms.line.prototype.highlight = function(context, convertToScreen) {
	if (context instanceof ms.view3d) {		
		var endpoints = this.getEndpoints();
		endpoints[0] && endpoints[0].highlight(context, convertToScreen);
		endpoints[1] && endpoints[1].highlight(context, convertToScreen);
		return;
	}
	for (var i = 0; i < this.getSegments().length; i++) {
		this.getSegments()[i].highlight(context, convertToScreen);
	}
};

ms.line.prototype.print = function() {
	ms.highlight(this);
};
