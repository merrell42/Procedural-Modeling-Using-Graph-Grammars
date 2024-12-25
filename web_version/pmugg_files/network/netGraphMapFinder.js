// This is similar to netMorphismFinder.
// TODO: Combine this, netGraphMap, and netGraphState with the netMorphisms files.
ms.netGraphMapFinder = function(nodeStats, groundEnabled) {
	this.nodeStats = nodeStats;
	this.nodesModified = false;
	this.groundFace = null;
	this.groundEnabled = groundEnabled;
};

ms.netGraphMapFinder.vertexAttempts = 100;

ms.netGraphMapFinder.faceAttempts = 30;

// Number of times to attempt to cast a series of rays to accomplish a splice.
ms.netGraphMapFinder.spliceRayAttempts = 3;

// The maximum distance the intermediate ray can be cast.
ms.netGraphMapFinder.maxRayDistance = 10;

// Find a map to netB.
ms.netGraphMapFinder.prototype.findMap = function(netB) {
	if (!this.groundFace && this.groundEnabled) {
		var faces = this.nodeStats.getElements('face');
		if (faces.length > 0) {
			this.groundFace = faces[0];
		}
	}
	
	var verticesB = netB.getInterior().getVertices();
	if (verticesB.length == 0) {
		return this.findStarterMap(netB);
	}

	// Start with a vertex that is not spliced.
	var index1 = 0;
	while (verticesB[index1].getPrimal().getType().getSpliced()) {
		index1++;
	}
	var primalB = verticesB[index1].getPrimal();
	
	var isConnector = !!primalB.getBoundary();
	var vertexType = primalB.getType();
	var verticesA = this.nodeStats.getElements('vertex');
	var N = verticesA.length;
	var attempts = Math.min(N, ms.netGraphMapFinder.vertexAttempts);
	var startIndex = ms.random(N);
	for (var i = 0; i < attempts; i++) {
		var vertexA = verticesA[(startIndex + i) % N];
		if ((isConnector || vertexA.getState().getType() == vertexType)
			 // Somehow restoring the nodeStats below can cause destroyed vertices to appear here. Filter them out.
			&& !vertexA.getNode().isDestroyed()) {
			this.nodesModified = false;
			var info = new ms.netGraphMapInfo(this.nodeStats, netB);
			var state = new ms.netGraphMapState(info);
			var map = this.assignVertex(state, vertexA, index1);
			if (map) {
				ms.netGraphMapFinder.addOuterFaces(map, netB);
				return map;
			} else if (this.nodesModified) {
				// Restore the previous values if the model was modified by split edges.
				this.nodeStats.restore();
			}
		}
	}
	return null;
};

ms.netGraphMapFinder.addOuterFaces = function(map, netB) {
	map.outerFaces = [];
	netB.getOuterFaces().forEach((outerFaceB, fIndex) => {
		var outerHalf = outerFaceB.outerComponent;
		var vIndex = netB.getInterior().getVertices().indexOf(outerHalf.getVertex());
		map.outerFaces[fIndex] = map.vertexBtoA[vIndex].getEndpoints()[outerHalf.vertexIndex].getFace();
	});
};

ms.netGraphMapFinder.prototype.findStarterMap = function(netB) {
	var info = new ms.netGraphMapInfo(this.nodeStats, netB);
	var state = new ms.netGraphMapState(info);
	
	var edges = netB.getBoundary().getEdges();
	if (edges == 0) {
		return state.map;
	}
	if (!this.groundEnabled || this.nodeStats.getCount('face') == 0) {
		return state.map;
	}
	
	var face = this.findFace(edges[0].getPrimal().getType());
	if (face) {
		state.map.outerFaces = [face];
		if (netB.boundary.faces[0].innerComponents.length > 0) {
			var goalFace = netB.boundary.faces[1];
			var intersectFace = null;
			var attempts = 0;
			while (attempts < ms.netGraphMapFinder.spliceRayAttempts && !intersectFace) {
				intersectFace = this.castVolumeRaySeries(face, netB.boundary.faces[0].innerComponents, goalFace);
				attempts++;
			}
			if (!intersectFace) {
				return null;
			} else {
				state.map.outerFaces.push(intersectFace);
			}
		}
		return state.map;
	} else {
		return null;
	}
};

// Find a face with the faceType that is not a hole.
ms.netGraphMapFinder.prototype.findFace = function(faceType) {
	if (this.groundEnabled && faceType == this.groundFace.getFaceType()) {
		if (ms.randomUniform(0, 1) < ms.globalSettings.get('Prefer Ground')) {
			return this.groundFace;
		}
	}
	
	var facesA = this.nodeStats.getElements('face');
	var N = facesA.length;
	var attempts = Math.min(N, ms.netGraphMapFinder.faceAttempts);
	var startIndex = ms.random(N);
	var options = [];
	var weights = [];
	for (var i = 0; i < attempts; i++) {
		var faceA = facesA[(startIndex + i) % N];
		if (faceA.getFaceType() == faceType && !faceA.isHole()) {
			weights.push(Math.abs(faceA.signedArea()));
			options.push(faceA);
		}
	}
	if (weights.length > 0) {
		return options[ms.pickByWeight(weights)];
	}
	return null;
};

// Fill the next empty vertex spot.
/* ms.netGraphMapFinder.prototype.assignFirstVertex = function(state, vertexA) {
	var { info } = state;

	var signB = vertexB.signature();
	var vIndexB = info.verticesB.indexOf(vertexB);
	var results = [];
	for (var vIndexA = 0; vIndexA < info.verticesA.length; vIndexA++) {
		// We assume none of network A's vertices have been used yet.
		if (info.vSignA(vIndexA) == signB) {
			results = results.concat(this.assignVertex(state.copy(), vIndexA, vIndexB));
		}
	}
	return results;
}; */

// Continue trying to find a match with the current state. Returns morphism if successful.
// Returns null, if unsuccessful.
ms.netGraphMapFinder.prototype.findContinue = function(state) {
	if (state.queue.length > 0) {
		var endpointData = state.queue.shift();
		return this.matchEndpoint(endpointData, state);
	} else {
		if (state.spliceQueue.length > 0) {
			var endpointData = state.spliceQueue.shift();
			return this.spliceEndpoint(endpointData, state);
		} else {
			return state.map;
		}
	}
};

ms.netGraphMapFinder.prototype.assignVertex = function(state, vertexA, indexB) {
	// var state = oldState.copy();
	state.assignVertex(vertexA, indexB);
	return this.findContinue(state);
};

ms.netGraphMapFinder.prototype.matchEndpoint = function(endpointData, state) {
	var { halfB, vertexA } = endpointData;
	var endpointsA = vertexA.getEndpoints();
	var edgeB = halfB.getEdge();
	if (!edgeB) {
		// This is not a real half edge.
		return this.findContinue(state);
	}
	var typeB = edgeB.getPrimal().getType();
	var endpointA = endpointsA.find((endpointA) => {
		return (endpointA.getEdgeType() == typeB) &&
			(endpointA.getIsAtStart() == halfB.getForward());
	});
	if (!endpointA) {
		return null;
	}
	// Do not use this endpoint if it is attached to a face with interior vertex and
	// it is not an outer face.
	if (ms.netGraphMapFinder.neighboringHoles(endpointA.getLine(), edgeB) && 
		halfB.getFace().getPrimal().getTurns() != 1) {
		return null;
	}
	return this.assignEndpoint(endpointA, halfB, state);
};

// Returns true if any of the faces neighboring the line have holes and the face
// is part of a loop. Faces part of a loop will be destroyed during the transition
// and the holes will be face to pop off the face.
ms.netGraphMapFinder.neighboringHoles = function(line, edgeB) {
	return line.getEndpoints().some((endpoint, index) => {
		var face = endpoint.getFace();
		var hasHoles = (face.getGroup().getFaces().length > 1) && !face.isHole();
		return hasHoles && edgeB.getHalfEdges()[index][0].isLoopy();
	});
};

ms.netGraphMapFinder.prototype.assignEndpoint = function(endpointA, halfB, state) {
	var { info, map } = state;
	var vertexB = halfB.getNext().getVertex();
	var vIndexB = info.verticesB.indexOf(vertexB);
	var primalB = vertexB.getPrimal();
	var isConnector = !!primalB.getBoundary();
	var vertexType = primalB.getType();
	if (!isConnector && vertexType.getSpliced() && !map.vertexBtoA[vIndexB]) {
		endpointA.getLine().fullSplit(ms.randomUniform(0, 1));
		this.nodesModified = true;
	}
	
	var lineA = endpointA.getLine();
	var edgeB = halfB.getEdge();
	var eIndexB = info.edgesB.indexOf(edgeB);
	map.edgeBtoA[eIndexB] = lineA;

	var vertexA = endpointA.next().getVertex();
	var vIndexA = map.vertexBtoA.indexOf(vertexA);
	if (vIndexA >= 0) {
		if (isConnector || vIndexA == vIndexB) {
			// We've already matched the vertex at the end of halfB.
			return this.findContinue(state);
		} else {
			// vertexA has been matched somewhere else.
			return null;
		}
	}

	// At this point we know endpointA and halfB have no match at their ends.
	if (isConnector || vertexA.getState().getType() == vertexType || vertexType.getSpliced()) {
		return this.assignVertex(state, vertexA, vIndexB);
	} else {
		return null;
	}
};

ms.netGraphMapFinder.splicedVertexTypes = {};

ms.netGraphMapFinder.getVertexType = function(edgeType) {
	if (!ms.netGraphMapFinder.splicedVertexTypes[edgeType.id]) {
		var vertexType = new ms.vertexType();
		var faceIds = []; // Unsure about this.
		vertexType.addEdge(edgeType, true, edgeType.getAngle(), faceIds);
		vertexType.addEdge(edgeType, false, edgeType.getAngle(), faceIds);
		ms.netGraphMapFinder.splicedVertexTypes[edgeType.id] = vertexType;
	}
	return ms.netGraphMapFinder.splicedVertexTypes[edgeType.id];
};

ms.netGraphMapFinder.SMALL_DISTANCE = 1e-8;

ms.netGraphMapFinder.prototype.castVolumeRaySeries = function(face, rayHalfs, goalFace) {
	if (rayHalfs.length > 1) {
		ms.alert('Multiple Rays not implemented yet.');
	}
	var startPos = face.randomPoint();
	var tree = this.nodeStats.model.getBspTree();
	var goalType = goalFace.outerComponent.edge.primal.type;

	// We model the ray as a long polygon. We intersect it with the polygons in the BSP tree.
	var faceType = face.getFaceType();
	var dir = rayHalfs[0].getDir();
	var u = faceType.u;
	var v = faceType.v;
	var endPos = dir.copy().scale(ms.intersector.FAR_DISTANCE).add(startPos);
	var dSmall = ms.netGraphMapFinder.SMALL_DISTANCE;
	var startPos2 = u.copy().scale(dSmall).add(startPos);
	var   endPos2 = u.copy().scale(dSmall).add(endPos);
	
	var points = [startPos, endPos, endPos2, startPos2];
	var d = v.dot(startPos);

	var plane = new ms.bspPlane(v, d);
	var fakePolygon = { getPoints: () => { return points; }};
	
	if (false) {
		// This is for visualizing the ray.
		var nodeStats = face.node.getStats();
		var edgeType = rayHalfs[0].edge.primal.type;
		// Everything about this function is hacky. The maxDim can screw up the earcut rendering. edgeType.faceData[1] just happens to work.
		var debugFace = ms.face.createFromPositions(points, nodeStats, edgeType);
		debugFace.highlight();
		debugFace.getNode().destroy();
	}
	var intersections = tree.add(plane, fakePolygon, true);
	var closest = {value: null, d: Infinity};
	intersections.forEach((intersection) => {
		var d = dir.dot(intersection.position);
		if (d < closest.d) {
			closest = {value: intersection, d};
		}
	});
	if (closest.value) {
		intersection = closest.value;		
		var nextFace = intersection.polygon.getFace();
		if (nextFace.getFaceType() == goalType && !nextFace.isHole()) {
			return nextFace;
		}
	}
	return null;
};

ms.netGraphMapFinder.castRay = function(p0, dir, groupA, maxDim) {
	var p1 = dir.copy().scale(ms.intersector.FAR_DISTANCE).add(p0);
	var dir2 = dir.dropDim(maxDim);

	var nearestIntersect = { distance: Infinity, face: null, data: null };
	groupA.getFaces().forEach((faceA) => {	
		var fPositions = faceA.getPositions();

		var intersections = ms.intersector.lineFaceIntersect(p0, p1, fPositions, maxDim);
		intersections.forEach((intersection) => {
			var distance = dir2.dot(intersection.pos);
			if (distance < nearestIntersect.distance) {
				nearestIntersect = { distance, face: faceA, data: intersection };
			}
		});
	});
	return nearestIntersect;
};

// Cast a series of rays.
ms.netGraphMapFinder.castRaySeries = function(halfB, startPos, groupA, maxDim) {
	var p0 = startPos;
	var nextB = halfB.getNext();
	
	while (true) {
		var dir = halfB.getDir();
		var nearestIntersect = ms.netGraphMapFinder.castRay(p0, dir, groupA, maxDim);

		if (!nearestIntersect.data) {
			return null;
		}
		
		if (nextB.isSpliced()) {
			var dir2 = dir.dropDim(maxDim);
			var scale = dir.length() / dir2.length();
			var maxDistance = Math.max(scale * nearestIntersect.distance, ms.netGraphMapFinder.maxRayDistance);
			var d = ms.randomUniform(0, maxDistance);
			p0 = dir.copy().scale(d).add(p0);
		} else {
			var nextA = nearestIntersect.face.getEndpoints()[nearestIntersect.data.index];
			var edgeTypeA = nextA.getEdgeType();
			var edgeTypeB = nextB.getEdge().getPrimal().getType();
			if (edgeTypeA != edgeTypeB) {
				return null;
			} else {
				return nextA;
			}
		}

		halfB = nextB;
		nextB = nextB.getNext();
	}
};

ms.netGraphMapFinder.prototype.spliceEndpoint = function(endpointData, state) {
	var { halfB, vertexA } = endpointData;
	var netB = state.info.netB;
	var endB = halfB;
	while (endB.isSpliced()) {
		endB = endB.getNext();
	}
	var vIndexB = state.info.verticesB.indexOf(endB.getVertex());
	if (state.map.vertexBtoA[vIndexB]) {
		return this.findContinue(state);
	}

	var faceTypeB = halfB.getEdge().getPrimal().getType().getFaceData()[0].type;
	var endpointA = vertexA.getEndpoints().find((endpoint) => {
		return endpoint.getFace().getFaceType() == faceTypeB;
	});
	// We can be on a completely incorrect vertex that doesn't have the face at all.
	if (!endpointA) {
		return null;
	}
	// Cast a ray, find the nearest intersection from all the faces in the group.
	var groupA = endpointA.getFace().getGroup();
	var maxDim = faceTypeB.getMaxDim()

	var startPos = vertexA.getPosition();
	var attempts = 0;
	var intersectEndpoint = null;
	while (attempts < ms.netGraphMapFinder.spliceRayAttempts && !intersectEndpoint) {
		intersectEndpoint = ms.netGraphMapFinder.castRaySeries(halfB, startPos, groupA, maxDim);
		attempts++;
	}
	if (!intersectEndpoint) {
		return null;
	}
	var vertexA0 = intersectEndpoint.getVertex();
	var vertexA1 = intersectEndpoint.next().getVertex();
	var vIndex0 = state.map.vertexBtoA.indexOf(vertexA0);
	var vIndex1 = state.map.vertexBtoA.indexOf(vertexA1);
	var eIndex  = state.map.edgeBtoA.indexOf(intersectEndpoint.getLine());

	if (vIndex0 < 0 && vIndex1 < 0 && eIndex < 0) {
		// Split the line once if the line and its vertices have not been matched.
		// The split position is going to change in the solver, so there is no reason to calculate it. Pick it randomly.
		var result = intersectEndpoint.getLine().fullSplit(ms.randomUniform(0, 1));
		this.nodesModified = true;
		return this.assignVertex(state, result.newVertex, vIndexB);
	} else if (vIndex0 >= 0 && vIndex1 >= 0 && eIndex >= 0) {
		// Split the line three times if the line and its vertices have already been matched.
		var isAtStart0 = intersectEndpoint.getIsAtStart();
		var vertexB0 = netB.getInterior().getVertices()[vIndex0];
		var vertexB1 = netB.getInterior().getVertices()[vIndex1];
		var isConnector0 = (vertexB0.getPrimal().connectorIndex() >= 0);
		var isConnector1 = (vertexB1.getPrimal().connectorIndex() >= 0);
		if (!(isConnector0 ^ isConnector1)) {
			ms.alert('Expected one of the vertices to be a connector.');
		}
		var connectorAtStart = isConnector0 ? isAtStart0 : !isAtStart0;
		this.nodesModified = true;

		// Split the line three times.
		var splitLines = [];
		var splitVertices = [];
		var lineToSplit = intersectEndpoint.getLine();
		for (var i = 0; i < 3; i++) {
			var result = lineToSplit.fullSplit(1 / (4 - i));
			result.split.lines[1]
			splitVertices.push(result.newVertex);
			splitLines.push(result.split.lines[0]);
			lineToSplit = result.split.lines[1];
			if (i == 2) {
				splitLines.push(lineToSplit);
			}
		}
		var vIndexCon = isConnector0 ? vIndex0 : vIndex1;
		if (connectorAtStart) {
			state.map.vertexBtoA[vIndexCon] = splitVertices[2]
			state.map.edgeBtoA[eIndex] = splitLines[3];
			return this.assignVertex(state, splitVertices[0], vIndexB);
		} else {
			state.map.vertexBtoA[vIndexCon] = splitVertices[0]
			state.map.edgeBtoA[eIndex] = splitLines[0];
			return this.assignVertex(state, splitVertices[2], vIndexB);
		}
	} else if (vIndex0 < 0 || vIndex1 < 0 || eIndex < 0) {
		ms.alert('Unsure what to do about a partial match');		
	}
};