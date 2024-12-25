ms.vertex = function(stats, position) {
	var properties = {
			collection: new ms.singleProperty('collection'),
			endpoint: new ms.alternativeArray('endpoint', /* required */ false, /* ordered */ false),
			lineState: new ms.requiredArray('lineState', /* required */ false, 2),
			position: new ms.valueProperty('position'),
			vertexState: new ms.singleProperty('vertexState'),
			face: new ms.singleProperty('face'),
	};
	this.node = new ms.node(this, stats, 'vertex', properties);
	this.node.setChangeHandler('vertexState', this.updateStats.bind(this));
	this.node.setChangeHandler('endpoint', this.onEndpointsChanged.bind(this)); // Either endpoints or lineStates are required.
	this.node.setDestroyHandler(this.onDestroy.bind(this));
	this.tileSeeds = {base: ms.random(100), increment: ms.random(100)};
	this.imageSeed = ms.random(100);

	this.node.setValue('position', position.copy());
	// this.cell = stats.getModel().cellFromPosition(position);
};

ms.vertex.vertexMergingEnabled = false;
ms.vertex.closeVertexThreshold = 0.5;
ms.vertex.closeVertexThreshold2 = ms.vertex.closeVertexThreshold * ms.vertex.closeVertexThreshold;

ms.vertex.prototype.getNode = function() {
	return this.node;
};

ms.vertex.prototype.onDestroy = function() {
	this.updateStats();
};

ms.vertex.prototype.transferLineStates = function(vertexB) {
	var lineStates = vertexB.getLineStates().slice();
	for (var i = 0; i < 2; i++) {
		if (lineStates[i]) {
			vertexB.getNode().disconnect(lineStates[i]);
			this.addLineState(lineStates[i], i);
		}
	}
};

ms.vertex.prototype.addLineState = function(lineState, index) {
	this.node.doubleInsert(1 - index, index, lineState);
};

ms.vertex.prototype.removeLineStates = function() {
	var lineStates = this.getLineStates().slice();
	lineStates[0] && this.node.disconnect(lineStates[0]);
	lineStates[1] && this.node.disconnect(lineStates[1]);
};

ms.vertex.prototype.resolveEndpoints = function() {
	var state = this.getState();
	state && state.resolveEndpoints();
};

ms.vertex.createWithState = function(stats, position, angle, scale, type, opt_primal) {
	var vertex = new ms.vertex(stats, position);
	var newState = new ms.vertexState(stats, vertex, angle, scale, type, opt_primal);
	newState.resolveEndpoints();
	return vertex;
};

ms.vertex.prototype.hasConflict = function() {	
	return this.getEndpoints().some(function(endpoint) {
		return endpoint.isConflicted();
	});
};

ms.vertex.prototype.updateStats = function() {
	var destroyed = this.node.isDestroyed();
	if (destroyed) {
		this.node.stats.removeVertex(this.node);
	} else {
		this.node.stats.addVertex(this.node);
	}
};

ms.vertex.prototype.onEndpointsChanged = function() {	
	if (this.getEndpoints().length == 0) {
		var lineStates = this.getLineStates();
		if (lineStates[0]) {
			lineStates[0].merge(lineStates[1]);
		}
		this.node.destroy();
	}
	this.updateStats();
};

ms.vertex.prototype.getEndpoints = function() {
	return this.node.get('endpoint');
};

ms.vertex.prototype.getEndpoint = function(index) {
	return this.getEndpoints()[index];
};

ms.vertex.prototype.getState = function() {
	return this.node.get('vertexState');
};

ms.vertex.prototype.getLineStates = function() {
	return this.node.get('lineState');
};

ms.vertex.prototype.getPosition = function() {
	return this.node.get('position');
};

ms.vertex.prototype.getId = function() {
	return this.node.getId();
};

// Merge vertexB into this vertex and then destroy it and any states it has.
// The connecting endpoint if it exists connect this vertex and vertexB and
// is also deleted.
ms.vertex.prototype.merge = function(params) {
	var vertexB = params.vertex;
	var connectingEndpoint = params.connectingEndpoint;

	var endpointsB = vertexB.getEndpoints().slice();
	var stateB = vertexB.getState();
	stateB && stateB.remove();
	for (var i = 0; i < endpointsB.length; i++) {
		var endpointB = endpointsB[i];
		if (endpointB != connectingEndpoint) {
			endpointB.getNode().connect(this);
			endpointB.getLine().reconstructFromEndpoints();
		}
	}
	connectingEndpoint && connectingEndpoint.getNode().destroy();
	vertexB.getNode().destroy();

	if (connectingEndpoint) {
		// Bad endpoints are part of a line that connected the two merged vertices.
		var badEndpoints = this.getEndpoints().filter(function(endpoint) {
			var endpoints = endpoint.getLine().getEndpoints();
			return endpoints[0].getVertex() == endpoints[1].getVertex();
		});
		badEndpoints.forEach(function(endpoint) {
			endpoint.getNode().destroy();
		});
	}
};

ms.vertex.prototype.setPosition = function(position) {	
	this.node.setValue('position', position.copy());
};

ms.vertex.prototype.isMutable = function() {
	return this.node.getStats().getModel().cellFromPosition(this.getPosition()).isMutable();
};

ms.vertex.prototype.isMoveable = function() {
	if (!this.isMutable()) {
		return false;
	}
	return !this.getEndpoints().some(function(endpoint) {
		return endpoint.getTwin() && !endpoint.getTwin().getVertex().isMutable();
	});
};

ms.vertex.prototype.fillHighlight = function(renderData) {
	this.getEndpoints().forEach(function(endpoint) {
		endpoint.fillHighlight(renderData);
	});
};

ms.vertex.prototype.highlight = function(context, convertToScreen) {
	this.getEndpoints().forEach(function(endpoint) {
		endpoint.highlight(context, convertToScreen);
	});
};

ms.vertex.prototype.fillFromEdges = function(renderData, endpoint0, endpoint1) {
	var brush0 = endpoint0.getEdgeType().getBrush();
	var brush1 = endpoint1.getEdgeType().getBrush();

	var tile = brush0.get('Image');			
	var flipped = brush0.get('Flip');
	if (tile && tile != 'None') {
		// var allData = renderData.getTextureData(tile);

		var start0 = endpoint0.getIsAtStart();
		var start1 = endpoint1.getIsAtStart();
		// We don't have a way of handling this if the edges flip direction.
		if (start0 == start1) {
			return;
		}
		
		var seg0 = endpoint0.getLineState().getCoordinates().getAngledEdges()[start0 ? 0 : 1];
		var seg1 = endpoint1.getLineState().getCoordinates().getAngledEdges()[start1 ? 0 : 1];
		// var firstSeg0 = start0; // ((ms.util.fixAngle(seg0.getAngle() - seg1.getAngle()) > 0) ^ start1);
		var firstSeg0 = flipped ^ (ms.util.fixAngle(seg0.getAngle() - seg1.getAngle()) > 0);
		var firstStart = firstSeg0 ? start0 : start1;
		if (firstStart && flipped) {
			firstSeg0 = !firstSeg0;
		}
		
		// seg0 = seg0.copy();
		// seg1 = seg1.copy();
		// The angle of seg1 is reversed inside of ms.lineStateCoordinates.getRenderData.
		// (flipped ^ !start0) && seg0.setAngle(seg0.getAngle() + Math.PI);
		// (flipped ^ start1) && seg1.setAngle(seg1.getAngle() + Math.PI);
		var renderType = firstStart ? ms.lineStateCoordinates.RENDER_TYPE.CONVEX : ms.lineStateCoordinates.RENDER_TYPE.CONCAVE;
		if (firstSeg0) {
			newData = ms.lineStateCoordinates.getRenderData(renderData, brush0, seg1, seg0, this.tileSeeds, renderType);
		} else {
			newData = ms.lineStateCoordinates.getRenderData(renderData, brush0, seg0, seg1, this.tileSeeds, renderType);
		}
		// ms.lineStateCoordinates.fastConcat(allData, newData);
	}
};

ms.vertex.prototype.fillRenderData = function(renderData) {
	var decoration = this.getState().getType().getDecoration();
	if (decoration && decoration.hasImage()) {
		var imageName = ms.decorationView.pickImage(decoration.get('Image'), this.imageSeed);
		var allData = renderData.getTextureData(imageName);
		var centerX =  decoration.get('Center X');
		var centerY = -decoration.get('Center Y');
		var w = decoration.get('Width') / 2;
		var h = decoration.get('Height') / 2;
		var center = this.getPosition().copy();
		center.move(centerX, centerY);

		// context.rotate(angle);
		ms.util.fastConcat(allData.texcoords, ms.lineStateCoordinates.DEFAULT_TEXCOORDS);
		var newPositions = [
			center.x - w, center.y + h,
			center.x - w, center.y - h,
			center.x + w, center.y + h,
			center.x + w, center.y + h,
			center.x - w, center.y - h,
			center.x + w, center.y - h
		];
		ms.util.fastConcat(allData.positions, newPositions);
	} else {
		var endpoints = this.getEndpoints();
		if (endpoints.length >= 2) {
			var n = endpoints.length;
			var filled = [];
			for (var i = 0; i < endpoints.length; i++) {
				var i1 = (i + 1) % n;
				var endpoint0 = endpoints[i];
				var endpoint1 = endpoints[i1];
				var brush0 = endpoint0.getEdgeType().getBrush();
				var brush1 = endpoint1.getEdgeType().getBrush();
				if (brush0 && brush0 == brush1 && !filled[i] && !filled[i1]) {
					this.fillFromEdges(renderData, endpoint0, endpoint1);
					filled[i] = true;
					filled[i1] = true;
				}
			}
		}
	}
};

ms.vertex.prototype.draw = function(context, convertToScreen) {
	var state = this.getState();
	var decoration = state && state.getType().getDecoration();
	if (decoration && decoration.hasImage()) {
		var scale = convertToScreen.scale;
		var pos = convertToScreen(this.getPosition());
		var imageName = decoration.get('Image');
		var centerX = scale * decoration.get('Center X');
		var centerY = scale * decoration.get('Center Y');
		var image = ms.decorationView.getImage(imageName, this.imageSeed);
		context.save();
		context.translate(pos.x + centerX, pos.y + centerY);
		// context.rotate(angle);
		var w = scale * decoration.get('Width');
		var h = scale * decoration.get('Height');
		context.translate(-w / 2, -h / 2);
		context.drawImage(image, 0, 0, w, h);
		context.restore();
	}
};

// Order the endpoints in clockwise order.
ms.vertex.prototype.compare = function(endpointA, endpointB) {
	return endpointA.getAngle() > endpointB.getAngle();
};

ms.vertex.prototype.print = function() {
	window.console.log(this.node.id + ': ' + this.getPosition().print());
	for (var i = 0; i < this.getEndpoints().length; i++) {
		window.console.log((this.getEndpoints()[i].getAngle() * 180 / Math.PI).toFixed(2));
	}
	ms.highlight(this);
};
