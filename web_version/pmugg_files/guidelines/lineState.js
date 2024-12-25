ms.lineState = function(stats, coordinates) {
	var properties = {
		cell: new ms.alternativeArray('cell', /* required */ false),
		vertex: new ms.requiredArray('vertex', /* required */ false, 2),
		border: new ms.requiredArray('border', /* required */ false, 2),
		lineSegment: new ms.singleProperty('lineSegment', /* required */ true),
		coordinates: new ms.valueProperty('coordinates'),
	};
	this.node = new ms.node(this, stats, 'lineState', properties);
	this.node.setValue('coordinates', coordinates);

	this.dirty = true;
	this.updateIfDirty();
};

// For some reason, this thickness used to be negative. I wrote that this prevents intersections we don't want
// but can't remember why.
ms.lineState.INTERSECTION_THICKNESS = 1e-6;

ms.lineState.prototype.getNode = function() {
	return this.node;
};

ms.lineState.prototype.getCells = function() {
	return this.node.get('cell');
};

ms.lineState.prototype.getSegment = function() {
	return this.node.get('lineSegment');
};

ms.lineState.prototype.getVertices = function() {
	return this.node.get('vertex');
};

ms.lineState.prototype.getBorders = function() {
	return this.node.get('border');
};

ms.lineState.prototype.getLine = function() {
	var segment = this.getSegment();
	return segment ? segment.getLine() : null;
};

ms.lineState.prototype.getCoordinates = function() {
	return this.node.get('coordinates');
};

ms.lineState.prototype.getPosition = function(isAtStart) {
	return this.getCoordinates().getAngledEdges()[isAtStart ? 0 : 1].getPosition();
};

ms.lineState.prototype.getAngle = function(isAtStart) {
	return this.getCoordinates().getAngledEdges()[isAtStart ? 0 : 1].getAngle();
};

ms.lineState.prototype.isMutable = function() {
	var cells = this.getCells();
	return (cells[0].isMutable() && cells[cells.length - 1].isMutable());
};

ms.lineState.prototype.isPartiallyMutable = function(fromStart) {
	var cells = this.getCells();
	return fromStart ? cells[0].isMutable() : cells[cells.length - 1].isMutable();
};

ms.lineState.prototype.updateIfDirty = function() {
	if (this.dirty) {
		var angledEdges = this.getCoordinates().getAngledEdges();
		var position0 = angledEdges[0].getPosition();
		var position1 = angledEdges[1].getPosition();
		
		this.u = position1.copy().minus(position0);
		this.u.normalize();
		this.v = new ms.vec2(this.u.y, -this.u.x);
		this.u0 = this.u.dot(position0);
		this.u1 = this.u.dot(position1);
		this.t1 = this.u1 - this.u0;
	}
};

// t parameterizes the line. The two endpoints are at t = 0 and t = t1.
// Returns the nearest point on the line as parameterized by t.
ms.lineState.prototype.getT = function(point) {
	this.updateIfDirty();
	return this.u.dot(point) - this.u0;
};

// Measures the distance from the given point to the nearest point on the line.
ms.lineState.prototype.lineDistance = function(point) {
	this.updateIfDirty();
	var position0 = this.getPosition(true);
	return this.v.dot(point) - this.v.dot(position0);
};

// Add to the model without checking for intersections.
ms.lineState.prototype.addToModel = function() {
	var self = this;
	return this.forEachCell(true, function(cell, tLower, tUpper) {
		cell.addState(self);
		return null;
	});
};

// Add to the model checking for intersections.
// minT is present when constructing a path of fake line states to finding missing fragments because
// we do not want to count the intersection with the egde where it came from. We want to be cautious about finding intersections.
// But when we are adding real line states to the model we want to be aggressive in finding intersections, so minT = -Infinity.
ms.lineState.prototype.addToModelWithIntersections = function(minT, fromStart, trim, opt_isValidIntersection) {
	var isValidIntersection = opt_isValidIntersection || function(state) {	return true; };
	
	// A list of states that have already been tested for an intersection.
	var checkedStates = [];
	if (fromStart) {
		var u = this.u;
	} else {
		var u = this.u.copy().scale(-1);
	}
	var self = this;
	// var tIntersection = this.t1 - 1e-9;
	var tIntersection = Infinity;
	var firstIntersection = null;
	var position0 = this.getPosition(fromStart);
	
	var handleIntersection = function(intersection, state) {		
		if (intersection && isValidIntersection(state)) {
			var t = self.getT(intersection);
			if (!fromStart) {
				t = self.t1 - t;
			}
			if ((minT < t) && (t < tIntersection - 1e-4)) {
				tIntersection = t;
				firstIntersection = {position: intersection, state: state};
			}
		}
	};
	return this.forEachCell(fromStart, function(cell, tLower, tUpper) {
		if (!cell || !cell.isMutable()) {
			var tNew = (tLower > ms.line.backtrack * 1.1) ? tLower - ms.line.backtrack : tLower / 2;
			var newPosition = u.copy().scale(tNew).add(position0);
			self.setPosition(newPosition, !fromStart);

			var border = cell && cell.getMutationBorder();
			if (border) {
				border.addState(self, newPosition);
			}
			return {position: newPosition, state: null};
		}
		cell.getActiveStates().forEach(function(stateB) {
			if (!checkedStates.includes(stateB)) {
				checkedStates.push(stateB);
				handleIntersection(self.intersect(stateB), stateB);
			}
		});
		cell.addState(self);
		if (firstIntersection && tIntersection < tUpper) {
			if (trim) {
				self.setPosition(firstIntersection.position, !fromStart);
			}
			return firstIntersection;
		}
		return null;
	});
};

// Find any line connections that this intersects. 
ms.lineState.prototype.findConnectionIntersections = function() {
	var faceConnections = new Set();
	this.forEachCell(true, function(cell, tLower, tUpper) {
		cell && cell.getFaceConnections().forEach(function(connection) {
			faceConnections.add(connection);
		});
	});
	var self = this;
	return Array.from(faceConnections).filter(function(connection) {
		return connection.intersectsState(self);
	});
};

ms.lineState.prototype.countIntersections = function() {
	// A list of states that have already been tested for an intersection.
	var checkedStates = [];
	var intersectionCount = 0;
	var u = this.u;
	var self = this;
	return this.forEachCell(true, function(cell, tLower, tUpper) {
		cell && cell.getActiveStates().forEach(function(stateB) {
			if (!checkedStates.includes(stateB)) {
				checkedStates.push(stateB);
				var intersection = self.intersect(stateB);
				if (intersection) {
					var t = self.getT(intersection);
					if ((1e-9 < t) && (t < self.t1 - 1e-9)) {
						intersectionCount++;
					}
				}
			}
		});
		return intersectionCount;
	});
};

ms.lineState.prototype.hasIntersections = function() {
	// A list of states that have already been tested for an intersection.
	var checkedStates = [];
	var u = this.u;
	var self = this;
	return this.forEachCell(true, function(cell, tLower, tUpper) {
		return cell && cell.getActiveStates().some(function(stateB) {
			if (!checkedStates.includes(stateB)) {
				checkedStates.push(stateB);
				var intersection = self.intersect(stateB);
				if (intersection) {
					var t = self.getT(intersection);
					if ((1e-9 < t) && (t < self.t1 - 1e-9)) {
						return true;
					}
				}
			}
			return false;
		});
	});
};

// If the line state hits another state or goes into an immutable region, it is trimmed.
// Return the trimmed position or null if the full line state was added.
ms.lineState.prototype.forEachCell = function(fromStart, func) {
	var model = this.node.getModel();
	var position = this.getPosition(fromStart);
	var x = position.x;
	var y = position.y;
	if (fromStart) {
		var u = this.u;
	} else {
		var u = this.u.copy().scale(-1);
	}
	var stepX = (u.x > 0) ? 1 : -1;
	var stepY = (u.y > 0) ? 1 : -1;
	var tDeltaX = Math.abs(1 / u.x);
	var tDeltaY = Math.abs(1 / u.y);
	var cellX = Math.floor(x);
	var cellY = Math.floor(y);

	if (Number.isInteger(x)) {
		var tMaxX = tDeltaX;
	} else {
		if (u.x > 0) {
			var tMaxX = (Math.ceil(x) - x) / u.x;
		} else if (u.x == 0) {
			var tMaxX = Infinity;
		} else {
			var tMaxX = (Math.floor(x) - x) / u.x;
		}
	}
	if (Number.isInteger(y)) {
		var tMaxY = tDeltaY;
	} else {
		if (u.y > 0) {
			var tMaxY = (Math.ceil(y) - y) / u.y;
		} else if (u.y == 0) {
			var tMaxY = Infinity;
		} else {
			var tMaxY = (Math.floor(y) - y) / u.y;
		}
	}
	var tLower = 0;
	while (true) {
		var cell = model.getCell(cellX, cellY);
		if (tMaxX < tMaxY) {
			tUpper = tMaxX;
			tMaxX += tDeltaX;
			cellX += stepX;
		} else {
			tUpper = tMaxY;
			tMaxY += tDeltaY;
			cellY += stepY;
		}
		var result = func(cell, tLower, tUpper);
		if (result) {
			return result;
		}		
		if (tUpper > this.t1) {
			return null;
		}
		tLower = tUpper;
	}
};

ms.lineState.prototype.intersect = function(stateB) {
	if (this == stateB) {
		// This can happen with the boundary for some reason.
		return null;
	}
	var thicknessA = this.getLine().getEdgeType().getThickness();
	var thicknessB = stateB.getLine().getEdgeType().getThickness();
	var intersection = this.getCoordinates().intersect(stateB.getCoordinates(), thicknessA, thicknessB);
	if (intersection) {
		// The intersection does not count if the line states share a vertex.
		var verticesA = this.getLine().getEndpoints().map(function(endpoint) { return endpoint.getVertex(); });
		var verticesB = stateB.getLine().getEndpoints().map(function(endpoint) { return endpoint.getVertex(); });
		if (verticesA[0] == verticesB[0] || verticesA[0] == verticesB[1] ||
			verticesA[1] == verticesB[0] || verticesA[1] == verticesB[1]) {
			return null;
		} else {
			return intersection;
		}
	} else {
		return null;
	}
};

ms.lineState.prototype.getCategory = function() {
	return 'default';
};

ms.lineState.prototype.destroy = function() {
	this.node.destroy();
};

ms.lineState.prototype.isDestroyed = function() {
	return this.node.isDestroyed();
};

// Find a random split point.
ms.lineState.prototype.getSplitPoint = function() {
	return this.getCoordinates().getSplitPoint();
};

// Get the length of the segment.
ms.lineState.prototype.getLength = function() {
	return this.getCoordinates().getLength();
};

// Return the position within the state where higher value are closer to the end vertex.
// NOTE: This could be simpler in the case where we are just ordering the position when splitting.
ms.lineState.prototype.statePosition = function(position) {
	return this.getT(position) / this.t1;
	/* var position0 = this.getCoordinates().getAngledEdges()[0].getPosition();
	var position1 = this.getCoordinates().getAngledEdges()[1].getPosition();
	var u = position1.copy().minus(position0);
	var u0 = u.dot(position0);
	var u1 = u.dot(position1);
	var u = u.dot(position);
	return (u - u0) / (u1 - u0); */
};

// Linearly interpolate between the coordinates.
ms.lineState.prototype.lerp = function(statePosition) {
	var position0 = this.getPosition(true);
	var position1 = this.getPosition(false);
	var u = position1.copy().minus(position0);
	return position0.copy().add(u.scale(statePosition));
};

// Linearly interpolate between the coordinate given a t-value.
ms.lineState.prototype.lerpT = function(t) {
	var position0 = this.getPosition(true);
	var result = this.u.copy();
	return result.scale(t).add(position0);
};

// Split the state and return the split off state.
ms.lineState.prototype.split = function(splitPoint) {
	var coordinates = this.getCoordinates().split(splitPoint);
	var states = [
		new ms.lineState(this.node.getStats(), coordinates[0]),
		new ms.lineState(this.node.getStats(), coordinates[1])
	];
	states[0].addToModel();
	states[1].addToModel();
	var vertices = this.getVertices().slice();
	var borders = this.getBorders().slice();
	for (var i = 0; i < 2; i++) {
		if (vertices[i]) {
			this.node.disconnect(vertices[i]);
			vertices[i].addLineState(states[i], 1 - i);
		}
		if (borders[i]) {
			borders[i].replaceState(this, states[i], i);
		}
	}
	return states;
};

// Merge two line states.
ms.lineState.prototype.merge = function(stateB) {
	var angledEdgeA =   this.getCoordinates().getAngledEdges()[0];
	var angledEdgeB = stateB.getCoordinates().getAngledEdges()[1];
	
	var coordinates = this.getCoordinates().copyWithAngledEdges([angledEdgeA, angledEdgeB]);
	var newState = new ms.lineState(this.node.getStats(), coordinates);
	newState.addToModel();
	var vertex0 = this.getVertices()[0];
	var vertex1 = stateB.getVertices()[1];
	if (vertex0) {
		this.node.disconnect(vertex0);
		vertex0.addLineState(newState, 1);
	}
	if (stateB.getVertices()[1]) {
		stateB.getNode().disconnect(vertex1);
		vertex1.addLineState(newState, 0);
	}
	var border0 = this.getBorders()[0];
	var border1 = stateB.getBorders()[1];
	if (border0) {
		border0.replaceState(this, newState, 0);
	}
	if (border1) {
		border1.replaceState(stateB, newState, 0);
	}
	this.getSegment().merge(stateB.getSegment(), newState);
	this.node.destroy();
	stateB.getNode().destroy();
};

ms.lineState.prototype.setPosition = function(newPosition, isAtStart) {
	var movedCoordinates = this.getCoordinates().copy();
	movedCoordinates.setPosition(newPosition, isAtStart);
	this.node.setValue('coordinates', movedCoordinates);
};

ms.lineState.prototype.move = function(startMovement, endMovement) {
	var movedCoordinates = this.getCoordinates().copy();
	movedCoordinates.getAngledEdges()[0].getPosition().move(startMovement.x, startMovement.y);
	movedCoordinates.getAngledEdges()[1].getPosition().move(endMovement.x, endMovement.y);
	this.node.setValue('coordinates', movedCoordinates);
	this.refreshCells(true);
};

ms.lineState.prototype.removeCells = function() {
	var oldCells = this.getCells().slice();
	var self = this;
	oldCells.forEach(function(cell) {
		cell.removeState(self);
	});
};

ms.lineState.prototype.refreshCells = function() {
	this.updateIfDirty();
	this.removeCells();
	this.addToModel();
};
	
ms.lineState.prototype.getEndpoint = function() {
	var line = this.getLine();
	var endpoint;
	if (line.getState(true) == this) {
		return line.getEndpoints()[0];
	} else if (line.getState(false) == this) {
		return line.getEndpoints()[1];
	} else {
		return null;
	}
};

ms.lineState.prototype.highlight = function(context, convertToScreen) {
	this.draw(context, convertToScreen, true);
};

ms.lineState.prototype.setColor = function(context, highlighted) {	
	var brush = this.getLine() ? this.getLine().getEdgeType().getBrush() : null;
	var brushColor = brush ? brush.getColor() : '#800';
	if (highlighted) {
		// This is pretty hacky way to work around a brush color that is close to the default highlighting color.
		var highlightColor = (brushColor != '#800') ? '#f0f' : '#0aa';
		context.strokeStyle = highlightColor;
		context.lineWidth = 4;
	} else {
		var brush = this.getLine() ? this.getLine().getEdgeType().getBrush() : null;
		context.strokeStyle = brushColor;
		// This is where the line thickness is actually set.
		context.lineWidth = 3;
	}
};

ms.lineState.prototype.fillHighlight = function(renderData) {
	this.dirty = true;
	this.fillRenderData(renderData, true);
};

ms.lineState.prototype.fillRenderData = function(renderData, opt_highlighted) {
	var line = this.getLine();
	var brush = line.getEdgeType().getBrush();
	var tileSeeds = line.getTileSeeds();
	var endpoints = line.getEndpoints();
	var vertices = endpoints.map(function(e) { return e.getVertex(); });
	
	// this.setColor(context, opt_highlighted || false);
	if (opt_highlighted) {
		brush = new ms.brush('ff00ff', '', () => {});
	}
	
	this.getCoordinates().fillRenderData(renderData, brush, tileSeeds);
};

ms.lineState.prototype.draw = function(context, convertToScreen, opt_highlighted) {
	var line = this.getLine();
	var rigid = line && line.isRigid();
	var brush = this.getLine() ? this.getLine().getEdgeType().getBrush() : null;
	this.setColor(context, opt_highlighted || false);
	this.getCoordinates().draw(context, rigid, brush, convertToScreen);
};

ms.lineState.prototype.print = function() {
	ms.highlight(this);
	return this.getCoordinates().print();
};
