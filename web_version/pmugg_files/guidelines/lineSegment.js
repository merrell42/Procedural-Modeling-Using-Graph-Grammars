ms.lineSegment = function(stats) {
	var properties = {
		lineState: new ms.requiredArray('lineState'),
		line: new ms.singleProperty('line', /* required */ true),
	};
	this.node = new ms.node(this, stats, 'lineSegment', properties);

	// The distance lower bound is the distance when it's below the 
	// ideal distance, beyond that the distance is not calculated.
	this.lengthLowerBound = 0;
	
	// Dirty length is currently not used.
	this.dirtyLength = true;
};

ms.lineSegment.moveMutable = false;

ms.lineSegment.prototype.getNode = function() {
	return this.node;
};

ms.lineSegment.prototype.getStates = function() {
	return this.node.get('lineState');
};

ms.lineSegment.prototype.getState = function(isAtStart) {
	var states = this.getStates();
	return states[isAtStart ? 0 : states.length - 1];
};

ms.lineSegment.prototype.getLine = function() {
	return this.node.get('line');
};

ms.lineSegment.prototype.addState = function(state, insertAtStart) {
	this.dirtyLength = true;
	this.node.connect(state, insertAtStart);
};

ms.lineSegment.prototype.addStates = function(states) {
	this.dirtyLength = true;
	var self = this;
	states.forEach(function(state) {
		self.node.connect(state);
	});
};

ms.lineSegment.prototype.destroy = function() {
	this.getStates().forEach(function(state) {
		state.getNode().destroy();
	});
};

ms.lineSegment.prototype.merge = function(segmentB, mergedState) {
	var statesA = this.getStates().slice();
	var statesB = segmentB.getStates().slice();
	// Throw out the two states that were merged and put in the mergedState.
	statesA[statesA.length - 1].getNode().disconnect(this);
	statesB.shift();
	statesB.unshift(mergedState);
	this.addStates(statesB);
	segmentB.node.disconnect(segmentB.getLine());
	segmentB.node.destroy();
};

ms.lineSegment.prototype.split = function(unsplitState, splitPoint, vertex, opt_index) {
	ms.timerG.start('split A');
	this.dirtyLength = 0;
	var index = opt_index || this.getStates().indexOf(unsplitState);
	if (index == -1) {
		ms.alert('Cannot find state to split.');
	}

	var newSegment = new ms.lineSegment(this.node.getStats());
	newSegment.addStates(this.getStates().slice(index + 1, this.getStates().length));

	var splitStates = unsplitState.split(splitPoint);
	this.addState(splitStates[0], false);
	newSegment.addState(splitStates[1], true);
	unsplitState.destroy();

	var line = this.getLine();
	var segmentIndex = line.getSegments().indexOf(this);
	line.getNode().splice(newSegment, segmentIndex + 1);

	vertex.addLineState(splitStates[0], 0);
	vertex.addLineState(splitStates[1], 1);
	ms.timerG.stop('split A');
};

// TODO: Combine with setPosition.
// This remove the line segment from the cells, but does not add them back.
ms.lineSegment.prototype.setPositionsOneState = function(positions) {
	var states = this.getStates();
	while (states.length > 1) {
		var state = states[states.length - 1];
		state.getNode().disconnect(this);
		state.destroy();
	}	
	states[0].setPosition(positions[0], true);
	states[0].setPosition(positions[1], false);
	states[0].refreshCells();
};

// TODO: Combine with setPosition.
// This remove the line segment from the cells, but does not add them back.
ms.lineSegment.prototype.setPositions = function(positions) {
	var states = this.getStates().filter(function(state) {
		return !ms.lineSegment.moveMutable || state.isMutable();		
	});	
	
	var p0 = states[0].getPosition(true);
	var p1 = states[states.length - 1].getPosition(false);
	var q0 = positions[0];
	var q1 = positions[1];
	var ua = p1.copy().minus(p0);
	var ub = q1.copy().minus(q0);
	var va = new ms.vec2(-ua.y, ua.x);
	var vb = new ms.vec2(-ub.y, ub.x);
	var A = new ms.matrix('full', [[ua.x, va.x], [ua.y, va.y]]);
	var B = new ms.matrix('full', [[ub.x, vb.x], [ub.y, vb.y]]);
	var P0 = new ms.matrix('full', [[p0.x], [p0.y]]);
	var BAi = B.dot(A.inv());
	var BAip0 = BAi.dot(P0);
	var offset = new ms.vec2(-BAip0.x() + q0.x, -BAip0.y() + q0.y);
	
	for (var i = 0; i < states.length; i++) {
		for (var atEnd = 0; atEnd < 2; atEnd++) {
			var p = states[i].getPosition(atEnd == 0);
			var P = new ms.matrix('full', [[p.x], [p.y]]);
			var BAip = BAi.dot(P);
			var newPosition = new ms.vec2(BAip.x() + offset.x, BAip.y() + offset.y);
			states[i].setPosition(newPosition, atEnd == 0);
			states[i].refreshCells();
		}
	}
};

ms.lineSegment.prototype.setPosition = function(newPosition, isAtStart) {
	var totalLength = 0;
	var cumulativeLengths = [0];
	var states = this.getStates().filter(function(state) {
		return !ms.lineSegment.moveMutable || state.isMutable();		
	});
	states.forEach(function(state) {
		totalLength += state.getLength();
		cumulativeLengths.push(totalLength);
	});
	if (totalLength == 0) {
		ms.alert('Zero mutable length.');
	}
	
	var state = states[isAtStart ? 0 : states.length - 1];
	var position0 = state.getPosition(isAtStart);
	var fullMovement = newPosition.copy().minus(position0);
	var movements = [];
	for (var i = 0; i < cumulativeLengths.length; i++) {
		var scale = (isAtStart ? totalLength - cumulativeLengths[i] : cumulativeLengths[i]) / totalLength;
		movements.push(fullMovement.copy().scale(scale));
	}
	for (var i = 0; i < states.length; i++) {
		states[i].move(movements[i], movements[i + 1]);
	}
};

ms.lineSegment.prototype.findIntersections = function() {
	return this.getStates().some(function(state) {
		return state.countIntersections() > 0;
	});
};

ms.lineSegment.prototype.getLength = function(idealLength) {
	ms.timerG.start('Segment Length');
	// Dirty length could be used here to speed things up, but some states may become immutable.
	// if (this.dirtyLength) {
	this.lengthLowerBound = 0;
	var states = this.getStates();
	for (var i = 0; i < states.length; i++) {
		if (!ms.lineSegment.moveMutable || states[i].isMutable()) {
			this.lengthLowerBound += states[i].getLength();
			if (this.lengthLowerBound >= idealLength) {
				break;
			}
		}
	}
	// 	this.dirtyLength = false;
	// }
	ms.timerG.stop('Segment Length');
	return this.lengthLowerBound;
};

ms.lineSegment.prototype.highlight = function(context, convertToScreen) {
	for (var i = 0; i < this.getStates().length; i++) {
		this.getStates()[i].highlight(context, convertToScreen);
	}
};

ms.lineSegment.prototype.print = function() {
	var states = this.getStates();
	window.console.log(states.length + ' states');
	states[0].print();
	states[states.length - 1].print();
	ms.highlight(this);
};
