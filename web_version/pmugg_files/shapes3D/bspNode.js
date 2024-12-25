ms.bspNode = function(stats, plane, polygons) {
	var properties = {
			bspPolygon: new ms.alternativeArray('bspPolygon', false),
			// There are three edges. 0 for the parent, 1 for the above child, 2 for the below child.
			bspEdge: new ms.requiredArray('bspEdge', false, 3),
	};
	this.node = new ms.node(this, stats, 'bspNode', properties);
	this.node.setChangeHandler('bspPolygon', this.onChanged.bind(this));
	this.node.setChangeHandler('bspEdge', this.onChanged.bind(this));
	
	var self = this;
	polygons.forEach(function(polygon) {
		self.addPolygon(polygon);
	});

	this.plane = plane;
};

ms.bspNode.prototype.getNode = function() {
	return this.node;
};

ms.bspNode.prototype.getPolygons = function() {
	return this.node.get('bspPolygon');
};

ms.bspNode.prototype.getAbove = function() {
	var edge = this.node.get('bspEdge')[1];
	return edge ? edge.getChild() : null;
};

ms.bspNode.prototype.getBelow = function() {
	var edge = this.node.get('bspEdge')[2];
	return edge ? edge.getChild() : null;
};

ms.bspNode.prototype.addPolygon = function(polygon) {
	this.node.connect(polygon);
};

ms.bspNode.prototype.onChanged = function() {
	if (this.getPolygons().length == 0 &&
		!this.getAbove() && !this.getBelow()) {
		this.node.destroy();	
	}
};

// This is only called on the root. The children are called using addNormal.
ms.bspNode.prototype.add = function(plane, polygon, isRayCast) {
	if (!this.plane) {
		this.plane = plane;
		this.addPolygon(polygon);
		return true;
	} else {
		return this.addNormal(plane, polygon, isRayCast);
	}
};

ms.bspNode.prototype.addAbove = function(plane, polygon, isRayCast) {
	var above = this.getAbove();
	if (above) {
		return above.addNormal(plane, polygon, isRayCast);
	} else {		
		if (isRayCast) {
			return [];
		}

		var stats = this.node.getStats();
		var edge = new ms.bspEdge(stats);
		var aboveNode = new ms.bspNode(stats, plane, [polygon]);
		edge.addNodes(this, aboveNode, true)
		return true;
	}
};

ms.bspNode.prototype.addBelow = function(plane, polygon, isRayCast) {
	var below = this.getBelow();
	if (below) {
		return below.addNormal(plane, polygon, isRayCast);
	} else {
		if (isRayCast) {
			return [];
		}

		var stats = this.node.getStats();
		var edge = new ms.bspEdge(stats);
		var belowNode = new ms.bspNode(stats, plane, [polygon]);
		edge.addNodes(this, belowNode, false)
		return true;
	}
};

ms.bspNode.prototype.addNormal = function(plane, polygon, isRayCast) {
	var points = polygon.getPoints();
	if (this.plane.isParallel(plane)) {
		if (this.plane.sameD(plane)) {
			if (isRayCast) {
				return [];
			}
			// TODO: Check that this works. Seems to be OK for now.
			/* if (this.getPolygons().length > 0) {
				ms.alert('Not sure if we should be adding multiple polygons to the plane.');
			} */
			this.addPolygon(polygon);
			return true;
		} else {
			if (this.plane.sign(points[0]) == 1) {
				return this.addAbove(plane, polygon, isRayCast);
			} else {
				return this.addBelow(plane, polygon, isRayCast);
			}
		}
	}
	var self = this;
	var signs = points.map(function(point) {
		return self.plane.sign(point);
	});
	var currentSign = 0;
	var firstSign = 0;
	// All the indices where it transitions between positive and negative.
	var transitions = [];
	for (var i = 0; i < signs.length; i++) {
		var s = signs[i]
		if (firstSign == 0) {
			firstSign = s;
		}
		if (currentSign == 0) {
			currentSign = s;
		} else if ((currentSign == 1 && s == -1) ||
		           (currentSign == -1 && s == 1)) {
			currentSign = s;
			transitions.push(i);
		}
	}
	if (currentSign != firstSign) {
		transitions.unshift(0);
	}
	if (currentSign == 0) {
		ms.alert('One point should be above or below the plane.');
	}
	// If there are no transitions all points are above or below the plane.
	if (transitions.length == 0) {
		if (currentSign == 1) {
			return this.addAbove(plane, polygon, isRayCast);
		} else {
			return this.addBelow(plane, polygon, isRayCast);
		}
	}
	if (transitions.length % 2 == 1) {
		ms.alert('There should be an even number of transitions.');
	}
	var N = points.length;
	var transitionPoints = [];
	for (var i = 0; i < transitions.length; i++) {
		var index = transitions[i];
		var prevIndex = (index + N - 1) % N;
		if (sign[index] == 0) {
			transitionPoints.push(points[index])
		} else if (sign[prevIndex] == 0) {
			transitionPoints.push(points[prevIndex])
		} else {
			transitionPoints.push(this.plane.crossingPoint(points[index], points[prevIndex]));
		}
	}
	
	// Sort the transition points along the line where the two planes
	// intersect. v is the direction of the line.
	var nA = plane.getN();
	var nB = this.plane.getN();
	var v = nA.cross(nB);
	var sortedPoints = transitionPoints.slice();
	sortedPoints.sort(function(a, b) {
		return v.dot(a) - v.dot(b);
	});
	var intersection = this.intersectsPolygon(sortedPoints, v, isRayCast);
	var intersections = [];
	if (intersection) {
		if (isRayCast) {
			intersections.push({position: sortedPoints[0], polygon: intersection});
		} else {
			return false;
		}
	}

	currentSign *= -1;
	var abovePoints = [];
	var belowPoints = [];
	for (var i = 0; i < transitions.length; i++) {
		var i1 = (i + 1) % transitions.length;
		var index0 = transitions[i];
		var index1 = transitions[i1];
		if (index1 < index0) {
			index1 += N;
		}
		var pointsToAdd = (currentSign == 1) ? abovePoints : belowPoints;
		pointsToAdd.push(transitionPoints[i]);
		for (var j = index0; j < index1; j++) {
			pointsToAdd.push(points[j % N]);
		}
		pointsToAdd.push(transitionPoints[i1]);
		currentSign *= -1;
	}
	if (isRayCast) {
		intersections = intersections.concat(this.addAbove(plane, polygon, isRayCast));
		intersections = intersections.concat(this.addBelow(plane, polygon, isRayCast));
		return intersections;
	} else {		
		var abovePolygon = new ms.bspPolygon(abovePoints, polygon.getFace());
		var belowPolygon = new ms.bspPolygon(belowPoints, polygon.getFace());
		var success = this.addAbove(plane, abovePolygon, isRayCast);
		return success && this.addBelow(plane, belowPolygon, isRayCast);
	}
};

ms.bspNode.prototype.intersectsPolygon = function(transitionPoints, v, isRayCast) {
	if (!isRayCast) {
		// Check if any of edges formed by the transitionPoints intersects any polygon edges.
		for (var i = 0; i < transitionPoints.length; i += 2) {
			var a0 = transitionPoints[i];
			var a1 = transitionPoints[i + 1];
			var intersects = this.getPolygons().some(function(polygon) {
				var points = polygon.getPoints();
				var n = points.length;
				for (var j = 0; j < n; j++) {
					var b0 = points[j];
					var b1 = points[(j + 1) % n];
					if (ms.intersector.intersect3D(a0, a1, b0, b1)) {
						return true;
					}
				}
			});
			if (intersects) {
				return true;
			}
		}
	}
	
	// Check if a point is completely inside the polygon. Cast a ray. If it intersects
	// the polygon an odd number of times, it is inside the polygon.
	var a0 = transitionPoints[0];
	var a1 = transitionPoints[0].copy().add(v.copy().scale(1e4));
	for (var i = 0; i < this.getPolygons().length; i++) {
		var polygon = this.getPolygons()[i];
		var points = polygon.getPoints();
		var n = points.length;
		var intersectionCount = 0
		for (var j = 0; j < n; j++) {
			var b0 = points[j];
			var b1 = points[(j + 1) % n];
			if (ms.intersector.intersect3D(a0, a1, b0, b1)) {
				intersectionCount++;
			}
		}
		var isOdd = (intersectionCount % 2 == 1);
		if (isOdd) {
			if (isRayCast) {
				return polygon;
			} else {
				return isOdd;
			}
		}
	};
};
