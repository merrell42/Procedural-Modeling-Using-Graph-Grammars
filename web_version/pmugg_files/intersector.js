ms.intersector = function() {};

// The minimum angle in degrees between two line segments for them to intersect.
// If they are within this angle they are close enough to being parallel that they
// can be ignored.
ms.intersector.MIN_ANGLE = 1e-6;
ms.intersector.MIN_CROSS_PRODUCT = Math.sin(ms.intersector.MIN_ANGLE);

// The thickness of the edges in pixels.
ms.intersector.THICKNESS = 0.1;

ms.intersector.intersect = function(s1, e1, s2, e2, opt_thickness1, options) {
  var thickness1 = (opt_thickness1 === undefined) ? ms.intersector.THICKNESS : opt_thickness1;
  var thickness2 = (options && options.thickness2 !== undefined) ? options.thickness2 : opt_thickness1;

  var r = e1.copy().minus(s1);
  var s = e2.copy().minus(s2);
  var rxs = r.crossZ(s);
  // If the cross product is below the minimum the line segments are almost parallel.
  if (Math.abs(rxs / r.length() / s.length()) < ms.intersector.MIN_CROSS_PRODUCT) {
	if (!options || !options.parallelIntersect) {
		return null;  
	}
	// If the two lines are separated by more than their thickness, they do not intersect.
	var q = new ms.vec2(-r.y, r.x);
	if (Math.abs(q.dot(s1) - q.dot(s2)) > thickness1 + thickness2) {
		return null;
	}
	// Orient the lines the same way.
	if (r.dot(s) < 0) {
		var temp = s2;		s2 = e2;		e2 = temp;
	}
	var rs1 = r.dot(s1);	var re1 = r.dot(e1);
	var rs2 = r.dot(s2);	var re2 = r.dot(e2);
	if (rs2 < rs1) {
		if (re2 < rs1) {
			// The second segment is all before the first.
			return null;
		} else {
			return s1;
		}
	} else {
		if (rs2 > re1) {
			// The second segment is all after the first.
			return null;
		} else {
			return s2;
		}
	}
  }
  
  var t =  s2.copy().minus(s1).crossZ(s) / rxs;
  var u = -s1.copy().minus(s2).crossZ(r) / rxs;

  var tWidth = thickness2 / r.length();
  var uWidth = thickness1 / s.length();
  // if ((0 < t) && (t < 1) && (0 < u) && (u < 1)) {
  if ((-tWidth < t) && (t < 1 + tWidth) && (-uWidth < u) && (u < 1 + uWidth)) {
    return r.scale(t).add(s1);
  } else {
    return null;
  }
};

// One line segment goes from a0 to a1. The other goes from b0 to b1.
// If they intersect, determine where they do.
ms.intersector.intersect3D = function(a0, a1, b0, b1) { // , opt_thickness1, opt_thickness2) {
	// var thickness1 = (opt_thickness1 === undefined) ? ms.intersector.THICKNESS : opt_thickness1;
	// var thickness2 = (opt_thickness2 === undefined) ? opt_thickness1 : opt_thickness2;

	var eFull = a1.copy().minus(a0);
	var fFull = b1.copy().minus(b0);
	var e = eFull.copy().normalize();
	var f = fFull.copy().normalize();
	var g = b0.copy().minus(a0);
	var h = f.cross(g);
	var k = f.cross(e);

	// Returns null if the two lines are parallel.
	if (k.length2() < ms.intersector.MIN_CROSS_PRODUCT) {
		return null;
	}
	
	var sign = (h.dot(k) > 0) ? 1 : -1;
	var p = a0.copy().add(e.copy().scale(sign * h.length() / k.length()));
	
	var t = p.copy().minus(a0).dot(e);
	var u = p.copy().minus(b0).dot(f);

	if ((0 < t) && (t + 1e-6 < eFull.length()) &&
		(0 < u) && (u + 1e-6 < fFull.length())) {
		return p;
	} else {
		return null;
	}
};

// It's bad to use global variables.
ms.intersector.tmpS = null;
ms.intersector.tmpE = null;
ms.intersector.tmpOffset = null;
ms.intersector.corners = null;

ms.intersector.initialize = function() {
	if (ms.intersector.tmpS == null) {
		ms.intersector.tmpS = new ms.vec2(0, 0);
		ms.intersector.tmpE = new ms.vec2(0, 0);
		ms.intersector.tmpOffset = new ms.vec2(0, 0);		
	}

	var corners = ms.intersector.corners;
	if (!corners) {
		corners = [];
		var w = 0.5;
		corners.push(new ms.vec2(-w, -w));
		corners.push(new ms.vec2( w, -w));
		corners.push(new ms.vec2(-w,  w));
		corners.push(new ms.vec2( w,  w));
		ms.intersector.corners = corners;
	}
	
};

ms.intersector.edgeShapeIntersect = function(edgeI, shapeJ, opt_thickness) {
	ms.intersector.initialize();
	var tmpS = ms.intersector.tmpS;
	var tmpE = ms.intersector.tmpE;
	var startI = edgeI.start.getPosition();
	var   endI = edgeI.end.getPosition();
	tmpS.x = startI.x;	tmpS.y = startI.y;
	tmpE.x =   endI.x;	tmpE.y = endI.y;
	
	var edgesJ = shapeJ.getEdges();
	for (var j = 0; j < edgesJ.length; j++) {
		var edgeJ = edgesJ[j];
		var intersection = ms.intersector.intersect(
				tmpS, tmpE, edgeJ.start.getPosition(), edgeJ.end.getPosition(), opt_thickness);
		if (intersection) {
			return true;
		}
	}
	return false;
};

ms.intersector.COPLANAR_DISTANCE = 1e-5;
// Project a ray a far distance that goes beyond the size of the model.
ms.intersector.FAR_DISTANCE = 10000;

// Are points a and b coplanar with respect to normal n.
ms.intersector.coplanar = function(a, b, n) {
	return Math.abs(n.dot(a) - n.dot(b)) < ms.intersector.COPLANAR_DISTANCE;
};

// Detect if a point is on a face with the given positions.
ms.intersector.onFace = function(fPositions, query0, n, dir) {
	if (!ms.intersector.coplanar(fPositions[0], query0, n)) {
		return false;
	}
	var maxDim = ms.util.maxDim(n);
	var query1 = query0.copy().add(dir.copy().scale(ms.intersector.FAR_DISTANCE));
	var intersections = ms.intersector.lineFaceIntersect(query0, query1, fPositions, maxDim);
	return (intersections.length % 2);
};

// Find all intersection between the line endpoints and the fPositions.
// The maxDim is dropped from the positions.
ms.intersector.lineFaceIntersect = function(line0, line1, fPositions, maxDim) {
	var N = fPositions.length;
	var fPositions2 = fPositions.map((pos) => {
		return pos.dropDim(maxDim);
	});
	var query0 = line0.dropDim(maxDim);
	var query1 = line1.dropDim(maxDim);

	var intersections = [];
	for (var i = 0; i < N; i++) {
		var v0 = fPositions2[i];
		var v1 = fPositions2[(i + 1) % N];
		var intersection = ms.intersector.intersect(v0, v1, query0, query1, 0);
		if (intersection) {
			intersections.push({pos: intersection, index: i});
		}
	}
	return intersections;
};

// Find the first intersection between the A and B face positions.
ms.intersector.faceIntersect = function(fPositionsA, fPositionsB, maxDim) {
	var fPositionsA2 = fPositionsA.map((pos) => {
		return pos.dropDim(maxDim);
	});
	var fPositionsB2 = fPositionsB.map((pos) => {
		return pos.dropDim(maxDim);
	});

	var Na = fPositionsA.length;
	var Nb = fPositionsB.length;
	var intersections = [];
	for (var i = 0; i < Na; i++) {
		var a0 = fPositionsA2[i];
		var a1 = fPositionsA2[(i + 1) % Na];
		for (var j = 0; j < Nb; j++) {			
			var b0 = fPositionsB2[j];
			var b1 = fPositionsB2[(j + 1) % Nb];
			var intersection = ms.intersector.intersect(a0, a1, b0, b1, 0);
			if (intersection) {
				return ({pos: intersection, indexA: i, indexB: j})
			}
		}
	}
	return null;
};
