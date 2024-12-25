// Edges
ms.edge = function(startVertex, endVertex) {
	this.start = new ms.exampleEndpoint(this, startVertex, true);
	this.end   = new ms.exampleEndpoint(this, endVertex, false);
	this.start.initialize();
	this.end.initialize();

	this.shape = null;
	this.selected = false;
	this.id = ms.edge.counter++;
	this.key = this.id;

	this.brush = null;
};

ms.edge.counter = 0;

ms.edge.COLOR = '#500';
ms.edge.SELECTED_COLOR = '#dd0';

ms.edge.prototype.getKey = function() {
	return this.key;
};

ms.edge.prototype.getId = function() {
	return this.id;
};

ms.edge.prototype.getBrush = function() {
	return this.brush;
};

ms.edge.prototype.setBrush = function(brush) {
	this.brush = brush;
};

ms.edge.prototype.setKey = function(key) {
	this.key = key;
};

ms.edge.prototype.compare = function(b) {
	return this.key - b.getKey();
};
	
ms.edge.prototype.getStart = function() {
	return this.start;
};

ms.edge.prototype.getEnd = function() {
	return this.end;
};

ms.edge.prototype.select = function() {
	this.selected = true;
};

ms.edge.prototype.deselect = function() {
	this.selected = false;
};

ms.edge.prototype.getSelected = function() {
	return this.selected;
};

ms.edge.prototype.setShape = function(shape) {
	this.shape = shape;
};

ms.edge.prototype.removeMe = function() {
	this.start.removeMe();
	this.end.removeMe();
};

ms.edge.prototype.drawArea = function(view, offset, options) {
	var color = options.color || ms.edge.COLOR;
	if (this.selected) {
		color = ms.edge.SELECTED_COLOR;
	}
	var v1 = !options.reverse ? this.start : this.end;
	var v2 =  options.reverse ? this.start : this.end;
	var bidirectional = this.brush ? this.brush.get('Bidirectional Edges') : true;
	options.hasArrows = (options.isArea || bidirectional) ? [false, false] : [false, true];
	options.brush = this.brush;
	view.drawLine(
			color,
			v1.getPosition(),
			v2.getPosition(),
			offset,
			options);
};

ms.edge.prototype.draw = function(view, offset, opt_color, secondPass) {
	view.context.lineWidth = 3;
	view.context.beginPath();
	var color = opt_color || (this.brush && this.brush.getColor());
	var flip = this.brush ? this.brush.get('Flip') : false;
	var reverse = ms.classifier.switchDirections(this) ^ flip;
	this.drawArea(view, offset, {color: color, reverse: reverse, secondPass: secondPass});
	view.context.stroke();
};

// Determines if the edge is straight to the left of the point v.
ms.edge.prototype.isLeft = function(v) {
	var x = v.getPosition().x;
	var y = v.getPosition().y;
	var xs = this.start.getPosition().x;
	var ys = this.start.getPosition().y;
	var xe = this.end.getPosition().x;
	var ye = this.end.getPosition().y;
	if ((ys > y) && (ye > y)) {
		// Edge is entirely above v.
		return false;
	}
	if ((ys < y) && (ye < y)) {
		// Edge is entirely below v.
		return false;
	}
	if (((ys == y) && (ye < y)) ||
	    ((ye == y) && (ys < y))) {
		// Without this we can double count a boundary when there
		// are two edges equal to y.
		return false;
	}
	var d = this.horizontalDistance(v);
	return (d > 0) && (d < Infinity);
	// var xedge = xs + (y - ys) * (xe - xs) / (ye - ys);
	// return (xedge < x);
};

ms.edge.prototype.distanceToPoint = function(point) {
	return this.lineDistanceToPoint(point);
};

ms.edge.prototype.horizontalDistance = function(point) {
	return this.lineHorizontalDistance(point);
};

ms.edge.prototype.lineHorizontalDistance = function(point) {
	var x1 = this.start.getPosition().x;
	var y1 = this.start.getPosition().y;
	var x2 = this.end.getPosition().x;
	var y2 = this.end.getPosition().y;
	var xp = point.getPosition().x;
	var yp = point.getPosition().y;

	var x = x1 - (y1 - yp) / (y1 - y2) * (x1 - x2);
	return xp - x;
};

// Returns the closest intercept that is to the left of the point.
ms.edge.prototype.intercept = function(position) {
	return this.lineIntercept(position);
};

// Determines if the edge is straight to the left of the point v.
ms.edge.prototype.lineIntercept = function(v) {
	var x = v.x;
	var y = v.y;
	var xs = this.start.getPosition().x;
	var ys = this.start.getPosition().y;
	var xe = this.end.getPosition().x;
	var ye = this.end.getPosition().y;
	if ((ys > y) && (ye > y)) {
		// Edge is entirely above v.
		return {vertices: [], tangents: []};
	}
	if ((ys < y) && (ye < y)) {
		// Edge is entirely below v.
		return {vertices: [], tangents: []};
	}
	if (((ys == y) && (ye < y)) ||
	    ((ye == y) && (ys < y))) {
		// Without this we can double count a boundary when there
		// are two edges equal to y.
		return {vertices: [], tangents: []};
	}

	var xResult = xs - (ys - y) / (ys - ye) * (xs - xe);
	var vertices = [new ms.exampleVertex(xResult, y)];
	var tangents = [this.tangent(0.5)];

	return {vertices: vertices, tangents: tangents};
};

ms.edge.prototype.lineDistanceToPoint = function(point) {
	var x1 = this.start.getPosition().x;
	var y1 = this.start.getPosition().y;
	var x2 = this.end.getPosition().x;
	var y2 = this.end.getPosition().y;
	var px = point.getPosition().x;
	var py = point.getPosition().y;
	// slope
	var m = (y2 - y1) / (x2 - x1);
	// y offset
	var b = y1 - m * x1;
	var d = [];
	if (x1 == x2) {
		// vertical line
		d.push(Math.abs(x1 - px));
	} else {
		// distance to the linear equation
		d.push(Math.abs(py - m * px - b) / Math.sqrt(Math.pow(m, 2) + 1));
	}
	// distance to p1
	d.push(Math.sqrt(Math.pow((px - x1), 2) + Math.pow((py - y1), 2)));
	// distance to p2
	d.push(Math.sqrt(Math.pow((px - x2), 2) + Math.pow((py - y2), 2)));
	// return the smallest distance
	return d.sort(function(a, b) {
		return a - b; //causes an array to be sorted numerically and ascending
	})[0];
};

ms.edge.prototype.getXs = function() {
	return [
			this.start.getPosition().x,
			this.start.getControlPosition().x,
			this.end.getControlPosition().x,
			this.end.getPosition().x];
};

ms.edge.prototype.getYs = function() {
	return [
			this.start.getPosition().y,
			this.start.getControlPosition().y,
			this.end.getControlPosition().y,
			this.end.getPosition().y];
};

// Get t for an endpoint.
ms.edge.prototype.getT = function(endpoint) {
	return (endpoint == this.start) ? 0 : 1;
};

ms.edge.prototype.tangent = function(t) {
	var dy;
	var dx;
	var startPos = this.start.getPosition();
	var endPos   = this.end.getPosition();
	dx = endPos.x - startPos.x;
	dy = endPos.y - startPos.y;
	return Math.atan2(dy, dx);
};

ms.edge.prototype.arcLength = function() {
	return this.start.distance(this.end);
};

ms.edge.prototype.windingNumber = function(fillPoint, isLeftSide) {
	var change = 0;
	var tangents = [];
	if (this.isLeft(fillPoint)) {
		tangents.push(this.getStart().tangent());
	}
	for (var i = 0; i < tangents.length; i++) {
		if (tangents[i] != 0) {
			if ((tangents[i] > 0) ? isLeftSide : !isLeftSide) {
				change++;
			} else {
				change--;
			}
		}
	}
	return change;
}

ms.edge.isAdjacent = function(e1, e2, offset1, offset2) {
	var a1 = e1.getStart();
	var b1 = e1.getEnd();
	var a2 = e2.getStart();
	var b2 = e2.getEnd();
	var adjPoints = ms.edge.adjacentPoints;
	return ((adjPoints(a1, a2, offset1, offset2) && adjPoints(b1, b2, offset1, offset2)) ||
	    (adjPoints(a1, b2, offset1, offset2) && adjPoints(b1, a2, offset1, offset2)));
};

ms.edge.adjacentPoints = function(v1, v2, offset1, offset2) {
	var pos1 = v1.getPosition();
	var pos2 = v2.getPosition();
	
	var x1 = pos1.x + offset1.x;
	var y1 = pos1.y + offset1.y;
	var x2 = pos2.x + offset2.x;
	var y2 = pos2.y + offset2.y;
	
	var dx = x1 - x2;
	var dy = y1 - y2;
	return (dx * dx + dy * dy < ms.discreteState.CONNECTION_DISTANCE2);
};

// Returns the uv coordinates relative to the edge for a query point.
ms.edge.prototype.edgeCoordinates = function(query) {
	var startPos = this.start.getPosition();
	var endPos = this.end.getPosition();
	var u = endPos.copy().minus(startPos);
	u.normalize();
	var v = new ms.vec2(-u.y, u.x);
	
	var su = startPos.dot(u);
	var sv = startPos.dot(v);
	var qu = query.dot(u);
	var qv = query.dot(v);
	var eu = endPos.dot(u);
	
	var u = (qu - su) / (eu - su);
	var v = qv - sv;
	return {u: u, v: v};
};

ms.edge.prototype.split = function(vertex) {
	var oldEnd = this.end;
	this.end = new ms.exampleEndpoint(this, vertex, false);
	this.end.initialize();
	var newEdge = new ms.edge(vertex, oldEnd.getVertex());
	newEdge.setBrush(this.brush);
	this.shape.addEdge(newEdge);
	oldEnd.removeMe();
};

// Returns true is the position is close. This is used for selection.
ms.edge.prototype.isClose = function(query, scale) {
	var coords = this.edgeCoordinates(query);
	return (0 < coords.u) && (coords.u < 1) && Math.abs(coords.v) < ms.shapeSet.NEAR_RADIUS / scale;
};

ms.edge.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.EDGE;
};

ms.edge.prototype.print = function () {
	window.console.log(this.start.getPosition().print() + '  ' + this.end.getPosition().print());
};
