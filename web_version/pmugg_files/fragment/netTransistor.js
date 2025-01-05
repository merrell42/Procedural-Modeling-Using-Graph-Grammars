ms.netTransistor = function() {};

// A global variable for handling matrix math.
var mathG;

ms.netTransistor.MAX_ANGLE_DIFFERENCE = 45 / 180 * Math.PI;

ms.netTransistor.buildNormally = function(transition, nodeStats, is3D) {
	var result = new ms.netTransistor();
	result.create(transition, nodeStats, is3D);
	if (result.effort > 0) {
		result.reject();
		return null;
	} else {
		return result;
	}
};

ms.netTransistor.prototype.create = function(transition, nodeStats, is3D) {
	this.startNet = transition.startNet;
	this.endNet = transition.endNet.removeSplices();
	this.map = transition.map;
	this.ground = transition.ground;
	this.dims = is3D ? 3 : 2;
	
	this.openPaths = [];
	this.lineData = [];
	this.lines = [];
	this.stats = nodeStats;
	this.model = this.stats.getModel();

	this.freeVertices = [];
	this.freeEdges = [];
	this.edgeBlockers = [];
	this.changeBasis = null;
	this.initialPosition = null;
	this.propagationOrder = null;
	this.basisEdges = [];
	this.isInitialBoundary = transition.initialBoundary;
	this.effort = 0;

	ms.timerG.start('Create Graph');
	// TODO: Merge duplicate lines.
	/* var success = this.mergeDuplicateLines();
	if (!success) {
		this.effort = Infinity;
		return;
	} */
	this.graph = this.createGraph();
	ms.timerG.stop('Create Graph');
	if (!this.graph) {
		this.effort = Infinity;
		return;
	}

	var edges = this.graph.edges;
	for (var i = 0; i < edges.length; i++) {
		this.addLine(edges[i], false, false);
	}
	// this.startInstance.destroy();
};

ms.netTransistor.maxEffort = 10;
ms.netTransistor.precision = 1e-8;
ms.netTransistor.constraintPrecision = 1e-5;
ms.netTransistor.minLength = 0;
ms.netTransistor.defaultLengthMin = 0.2;
ms.netTransistor.defaultLengthMax = 4;

ms.netTransistor.prototype.addLine = function(line, includeLength, addToGraph) {	
	var angle = line.getEdgeType().getAngle();
	var datum = {
		v: ms.vec2.unitVec(angle),
		line: line,
	};
	var lineEndpoints = line.getEndpoints();
	if (includeLength) {
		var v = lineEndpoints[1].getPosition().copy().minus(lineEndpoints[0].getPosition());
		// v.normalize(); // You could use it for the angle.
		datum.length = v.length();
	}
	this.lineData.push(datum);
	this.lines.push(line);

	var self = this;
	if (addToGraph) {
		var vertices = this.graph.vertices;
		var lineVertices = lineEndpoints.forEach(function(endpoint) {
			ms.union(self.graph.vertices, [endpoint.getVertex()]);
		});
		this.graph.edges.push(line);
	}
};

ms.netTransistor.prototype.createGraph = function() {
	var endNetwork = this.endNet.getInterior();
	var endVertices = endNetwork.getVertices();
	var endEdges = endNetwork.getEdges();
	var merged = { vertices: [], edges: []};
	var startNet = this.startNet;

	// Save the face location before we destroy the vertices.
	if (this.map.outerFaces) {
		this.map.outerFacesD = [];
		this.map.outerFaces.forEach((outerFace, index) => {
			this.map.outerFacesD[index] = outerFace.getFaceType().getNormal().dot(outerFace.getEndpoints()[0].getPosition());
		});
	}

	// Maps from half edges to merge endpoints.
	var mergedEndpoints = [];
	var halfToEndpoint = function(half) {
		var index = endNetwork.getHalfEdges().indexOf(half);
		return mergedEndpoints[index];
	}
	var setHalfToEndpoint = function(half, endpoint) {
		var index = endNetwork.getHalfEdges().indexOf(half);
		/* if (index == -1) {
			debugger;
		} */
		mergedEndpoints[index] = endpoint;
	};	

	var splitLines = [];
	var splitEndpoints = [];
	if (this.map) {
		splitLines = this.map.edgeBtoA.map(function(line) { return [line]});
	}
	this.angle = 0;
	
	for (var i = 0; i < endVertices.length; i++) {
		var v = endVertices[i];
		var primal = v.getPrimal();
		if (primal.connectorIndex() < 0) {
			var type = v.getPrimal().getType();
			var randomPosition = this.dims == 2 ?
				new ms.vec2(5 * Math.random(), 5 * Math.random()) :
				new ms.vec3(5 * Math.random(), 5 * Math.random(), 5 * Math.random());
			var newVertex = ms.vertex.createWithState(this.stats, randomPosition, this.angle, 1, type);
			merged.vertices[i] = newVertex;
			var vEndpoints = newVertex.getEndpoints();
			var halfs = primal.getInterior().getHalfEdges();
			for (var j = 0; j < halfs.length; j++) {
				setHalfToEndpoint(halfs[j], vEndpoints[j]);
			}
		}
	}
	ms.taskDebug();
	
	var edgeData = [];
	// A mapping from the core vertex to the graph vertices. Just for the outer vertices.
	var coreToGraphV = {};
	for (var i = 0; i < endEdges.length; i++) {
		var endEdge = endEdges[i];
		var edgeHalfs = endEdge.getHalfEdges();
		var coreEndpoints = [];
		var halfEdges = [];
		var modified = false;
		for (var e = 0; e < edgeHalfs.length; e++) {
			var half = edgeHalfs[e][0];
			var hVertex = half.getVertex();
			var connectorIndex = hVertex.getPrimal().connectorIndex();
			var core = merged.vertices[endVertices.indexOf(hVertex)];

			// For each connector find the matching endpoint in the original graph (in the instance).
			// For the other endpoints, just save the data we have for them.
			if (connectorIndex >= 0) {
				var startIndex = startNet.getInterior().getEdges().indexOf(
					startNet.getConnectors()[connectorIndex].interiorEdge());
				if (splitLines[startIndex].length == 1) {
					if (splitLines[startIndex][0].getNode().isDestroyed()) {
						ms.alert('The same line is being split twice. This should ' +
								 'not happen. Exiting early may not work.');
						return null;
					}
					var split = splitLines[startIndex][0].split();
					splitLines[startIndex] = split.lines;
					splitEndpoints[startIndex] = split.nextEndpoints;
				}
				var lineIndex = half.getForward() ? 0 : 1
				var coreEndpoint = splitLines[startIndex][lineIndex].getEndpoints()[e];
				splitLines[startIndex][lineIndex] = null;
				coreEndpoints[e] = coreEndpoint;
				
				// Check if the graph vertex has already been created.
				// This happens when two of the outer vertices match.
				var coreVertex = coreEndpoint.getVertex();
				var coreId = coreVertex.getNode().getId();
				var graphVertex = coreToGraphV[coreId];
				if (!graphVertex) {
					graphVertex = new ms.graphVertex(coreVertex);
					coreToGraphV[coreId] = graphVertex;
				}
				// coreEndpoints.push(coreEndpoint);
				halfEdges.push(half);
				modified = true;
				setHalfToEndpoint(half, coreEndpoint);
			} else {
				var coreEndpoint = core.getEndpoints()[half.getVertexIndex()];
				coreEndpoints.push(coreEndpoint);
				halfEdges.push(half);
			}
		}
		edgeData.push({coreEndpoints, halfEdges, modified: modified});
	}
	// Set the half edge at the end of each face.
	for (var i = 0; i < endEdges.length; i++) {
		var endEdge = endEdges[i];
		var edgeHalfs = endEdge.getHalfEdges();	
		for (var e = 0; e < edgeHalfs.length; e++) {
			var halfNext = edgeHalfs[e][0].getNext();
			if (!halfNext.getEdge()) {
				var hVertex = halfNext.getVertex();
				var connectorIndex = hVertex.getPrimal().connectorIndex();
				var index = startNet.getInterior().getEdges().indexOf(startNet.getConnectors()[connectorIndex].interiorEdge())
				setHalfToEndpoint(halfNext, splitEndpoints[index][e]);
			}
		}
	}
	
	
	var failed = false;
	endNetwork.getFaces().forEach(function(face) {
		var halfs = face.getOuterHalfEdges();
		var N = halfs.length;
		for (var i = 0; i < N; i++) {
			var endpointA = halfToEndpoint(halfs[i]);
			var endpointB = halfToEndpoint(halfs[(i + 1) % N]);
			if (!endpointA || !endpointB) {
				failed = true;
				continue;
			}
			endpointA.mergeFaces(endpointB);
		}
	});
	if (failed) {
		ms.alert('Do not know how this can happen, but halfToEndpoint is missing an endpoint.');
		return null;
	}
	
	
	var extents = this.model.getExtents();
	var debugCell = this.model.getCell(extents[0] - 1, extents[1] - 1);
	for (var i = 0; i < edgeData.length; i++) {
		var datum = edgeData[i];
		var line0 = datum.coreEndpoints[0].getLine();
		for (var j = 1; j < datum.coreEndpoints.length; j++) {
			var endpointJ = datum.coreEndpoints[j];
			var lineJ = endpointJ.getLine();
			line0.addEndpoint(endpointJ, datum.halfEdges[j].getEdgeIndex());
			lineJ.getNode().destroy();
		}
		line0.fillFromEndpoints();
		// debugCell.addState(line0.getSegment().getStates()[0]);
		merged.edges[i] = line0;
		
		if (datum.modified) {
			ms.union(merged.vertices, [datum.coreEndpoints[0].getVertex(), datum.coreEndpoints[1].getVertex()]);
		}
	}
	
	var endConnectors = this.endNet.getConnectors();
	for (var i = 0; i < endConnectors.length; i++) {
		var half = endConnectors[i].getInterior().getHalfEdges().find((half) => { return half.getEdge(); });
		var faceHalfs = ms.faceNet.getConnectedHalfEdges(half);
		var endHalf = faceHalfs.pop();
		var faceEndpointsI = faceHalfs.map(halfToEndpoint);

		var path = ms.transistorPath.createNet(faceEndpointsI, merged.edges, this.lines);
		var pathEnd = halfToEndpoint(endHalf);
		var pathEndpoints = [faceEndpointsI[0], pathEnd];
		path.setEndpoints(pathEndpoints);
		this.openPaths.push(path);
	}

	var connectionsToUpdate = new Set();
	addConnections = function(endpoint) {
		endpoint && endpoint.getConnections().forEach(function(connection) {
			connectionsToUpdate.add(connection);
		});
	};
	splitLines.forEach(function(lineGroup) {
		lineGroup.forEach(function(line) {
			if (line) {
				addConnections(line.getEndpoints()[0]);
				addConnections(line.getEndpoints()[1]);
				line.getNode().destroy();
			}
		});
	});

	merged.faces = [];
	endNetwork.getFaces().forEach((face) => {
		// The half edge is missing from some outer faces. This is a slight hack.
		var half = face.getOuterComponent();
		if (half) {
			merged.faces.push( halfToEndpoint(half).getFace());
		}
	});

	this.endNet.getOuterFaces().forEach((face) => {
		var faceIndex = this.endNet.getInterior().getFaces().indexOf(face);
		merged.faces[faceIndex].setHole(true);
	});
	merged.faces.forEach((face) => {
		// Split the face off from a group if it is not a hole, but is not in
		// outer position. It was split from another face, but is no longer coplanar.
		if (!face.isHole() &&
			face.getGroup().getFaces().indexOf(face) > 0) {
			face.splitGroup();
		}
	});
	this.map.outerFacesA = [];
	this.endNet.getOuterFaces().forEach((endFace, index) => {
		this.map.outerFacesA[index] = halfToEndpoint(endFace.getOuterComponent()).getFace();
	});
	
	
	var success = true;
	connectionsToUpdate.forEach(function(connection) {
		if (connection.getNode().isDestroyed()) {
			return;
		}
		var endpoints = connection.getEndpoints();
		var face = connection.getFace();
		// connection.getNode().destroy();
		if (endpoints[1]) {
			success = success && face.updateConnection();
		}
	});
	if (success) {
		return merged;
	} else {
		return null;
	}
};

ms.netTransistor.prototype.solve = function(mutationArea) {
	this.setup(mutationArea);
	if (this.changeBasis) {
		return this.sampleSolutionSpace(mutationArea);
	} else {
		return success;
	}
};

ms.netTransistor.prototype.setup = function(mutationArea) {
	if (ms.globalSettings.get('Fast Matrix Math')) {
		mathG = ms.fastMath;
	} else {
		mathG = math;
	}
	
	ms.timerG.start('Setup');
	var success = this.setupFaceCentric(mutationArea);
	ms.timerG.stop('Setup');
	return success;
};

ms.netTransistorSettings = function(mutationArea) {
	this.newFaceCounter = -1;
	this.vertexPlacements = {};
	this.edgePlacements = {};
	this.facePlacements = {};
	this.basisIds = [];
	this.lower = mutationArea.lowerExtent;
	this.upper = mutationArea.upperExtent;
	this.uniqueFaceMap = {};
	
	this.orderIds = [];
	this.orderInfo = [];
};

ms.netTransistorSettings.prototype.getVertex = function(id) { return this.vertexPlacements[id]; };
ms.netTransistorSettings.prototype.getEdge   = function(id) { return this.edgePlacements[id]; };
ms.netTransistorSettings.prototype.getFace   = function(id) { return this.facePlacements[id]; };

ms.netTransistorSettings.prototype.setVertex = function(id, vPlace) { this.vertexPlacements[id] = vPlace; };
ms.netTransistorSettings.prototype.setEdge   = function(id, ePlace) { this.edgePlacements[id] = ePlace; };
ms.netTransistorSettings.prototype.setFace   = function(id, fPlace) { this.facePlacements[id] = fPlace; };

ms.netTransistorSettings.prototype.addToOrder = function(id, type, vertexId) {
	if (!this.orderIds.includes(id)) {
		if (vertexId === undefined && type == 'face') {
			debugger;
		}
		this.orderIds.push(id);
		this.orderInfo.push({type, vertexId});
	}
};

ms.netTransistorSettings.prototype.createFace = function(normal) {
	var id = this.newFaceCounter--;
	this.facePlacements[id] = new ms.facePlacement(normal, id, this);
	return id;
};

// Merge face ID B into face ID A. The two faces are coplanar and cannot move independently.
ms.netTransistorSettings.prototype.mergeFace = function(idA, idB) {
	this.uniqueFaceMap[idB] = idA;
	var self = this;
	Object.keys(this.uniqueFaceMap).forEach((key) => {
		if (self.uniqueFaceMap[key] == idB) {
			self.uniqueFaceMap[key] = idA;
		}
	});
	Object.keys(this.vertexPlacements).forEach((key) => {
		var vPlace = self.getVertex(key);
		for (var i = 0; i < vPlace.freeFaceIds.length; i++) {
			if (vPlace.freeFaceIds[i] == idB) {
				vPlace.freeFaceIds[i] = idA;
				var vId = parseInt(key);
				ms.remove(vId, self.getFace(idB).vertexIds);
				self.getFace(idA).vertexIds.push(vId);
			}
		}
		for (var i = 0; i < vPlace.unfreeFaceIds.length; i++) {
			if (vPlace.unfreeFaceIds[i] == idB) {
				vPlace.unfreeFaceIds[i] = idA;
				var vId = parseInt(key);
				ms.remove(vId, self.getFace(idB).vertexIds);
				self.getFace(idA).vertexIds.push(vId);
			}
		}
	});
};

ms.netTransistor.constrainVertexIds = function(vIds, settings) {
	while (vIds.length > 0) {
		var newVIdsToContrain = [];
		// Find the most constrained vertex.
		var mostConstrained = {vPlace: null, num: -1};
		vIds.forEach((id) => {
			var vPlace = settings.vertexPlacements[id];
			var num = vPlace.getNumConstraints();
			if (mostConstrained.num < num && num < 3) {
				mostConstrained = {vPlace, num};
			}
			if (num < 3) {
				newVIdsToContrain.push(id);
			}
		});
		mostConstrained.vPlace && mostConstrained.vPlace.addConstraint();
		vIds = newVIdsToContrain;
	}
};

// Fixed face A is the one that's fixed and can be outside of the transition.
// Fixed face B is  part of the transition and is coplanar with face A.
// The two faces can be the same face.
ms.netTransistor.prototype.addFixedFace = function(fixedFaceA, fixedFaceB, d) {
	var fPlace = this.settings.getFace(fixedFaceB.getNode().getId())
	var alreadyFixed = this.fixedFaces.find((fixed) => {
		return fixed.fPlace == fPlace;
	});
	if (alreadyFixed) {
		return;
	}

	var fixedFace = {faceA: fixedFaceA, fPlace, d};
	this.fixedFaces.push(fixedFace);
	fPlace.makeFixed(fixedFace);
};

ms.netTransistor.prototype.setupFaceCentric = function(mutationArea) {
	this.changeBasis = true;
	this.isDiscretized = null;
	this.mutationArea = mutationArea;
	this.freeVertices = [];
	
	// TODO: Handle fixed sized edges.
	
	var self = this;
	var settings = new ms.netTransistorSettings(mutationArea);
	this.settings = settings;
	
	var basisIds = [];
	var vertexIds = [];
	
	this.graph.edges.forEach(function(e) {
		var id = e.getNode().getId();
		settings.edgePlacements[id] = new ms.edgePlacement(e, id, settings);
	});
	this.graph.vertices.forEach(function(v) {
		var id = v.getId();
		vertexIds.push(id);
		self.freeVertices.push(v);
		settings.vertexPlacements[id] = new ms.vertexPlacement(v, id, settings);
		settings.vertexPlacements[id].initialize();
	});
	this.graph.edges.forEach(function(e) {
		var id = e.getNode().getId();
		settings.edgePlacements[id].initialize();
	});
	vertexIds.forEach((id) => {
		settings.getVertex(id).checkThreeFaces();
	});

	// The vertices that need to be fixed.
	this.fixedVertexIds = [];
	for (var i = 0; i < this.openPaths.length; i++) {
		var path = this.openPaths[i];
		for (var j = 0; j < 2; j++) {
			var pathVertex = path.endpoints[j].getVertex();
			var id = pathVertex.getNode().getId();
			if (!this.fixedVertexIds.includes(id)) {
				this.fixedVertexIds.push(id);
			}
		}
	}

	this.fixedFaces = [];
	var outerFaces = this.map.outerFaces || [];
	outerFaces.forEach((fixedFaceA, index) => {
		// The case where endFace is missing is handled below when checking for groups.
		var endFace = this.endNet.getOuterFaces()[index];
		if (endFace) {
			var faceIndex = self.endNet.getInterior().getFaces().indexOf(endFace);
			var fixedFaceB = self.graph.faces[faceIndex];
			var d;
			// Use the value from outerFacesA if fixedFaceA is destroyed.
			if (fixedFaceA.getNode().isDestroyed()) {
				// Is this always the same as fixedFaceB?
				fixedFaceA = this.map.outerFacesA[index];
				d = this.map.outerFacesD[index];
			} else {
				var normal = fixedFaceB.getFaceType().getNormal();
				var vPosition = fixedFaceA.getEndpoints()[0].getVertex().getPosition();
				d = normal.dot(vPosition);
			}
			
			self.addFixedFace(fixedFaceA, fixedFaceB, d);
		}
	});
	Object.values(this.settings.facePlacements).forEach((fPlace) => {
		var face = fPlace.face;
		if (face) {
			var group = face.getGroup();
			if (group.getFaces().length > 1) {
				var normal = face.getFaceType().getNormal();
				var vPosition = face.getEndpoints()[0].getVertex().getPosition();
				var d = normal.dot(vPosition);
				this.addFixedFace(face, face, d);
			}			
		}
	});
	// this.settings.facePlacements

	ms.netTransistor.constrainVertexIds(this.fixedVertexIds, settings);
	ms.netTransistor.constrainVertexIds(vertexIds, settings);

	if (this.dims == 2) {
		this.propagationOrder = [];
		this.graph.edges.forEach(function(e) {
			self.propagationOrder.push(e.getEndpoints()[0]);
		});
	}
};

ms.netTransistor.prototype.findLimits = function(mutationArea) {
	var minLimit = [];
	var maxLimit = [];
	var vertexCols = this.dims * this.freeVertices.length;
	for (var i = 0; i < this.freeVertices.length; i++) {
		for (var dim = 0; dim < this.dims; dim++) {
			minLimit.push(mutationArea.lowerExtent[dim]);
			maxLimit.push(mutationArea.upperExtent[dim]);
		}
	}
	for (var i = 0; i < this.freeEdges.length; i++) {
		var edgeType = this.freeEdges[i].getEdgeType();
		var brush = edgeType.getBrush();
		var minLength = ms.netTransistor.minLength;
		var maxLength = Infinity;
		if (brush && brush.get('Strict Length')) {
			minLength = brush.get('Min Length');
			maxLength = brush.get('Max Length');
		}
		minLimit.push(minLength);
		maxLimit.push(maxLength);
	}
	return {min: minLimit, max: maxLimit};
};

ms.netTransistor.prototype.hasViolations = function(positions, limits) {
	for (var i = 0; i < positions.length; i++) {
		var value = positions[i][0];
		if (value < limits.min[i] || value > limits.max[i]) {
			return true;
		}
	}
	return false;
};

ms.netTransistor.prototype.getRange = function(orderIds, orderInfo) {	
	var settings = this.settings;
	var range = new ms.range(-Infinity, Infinity);
	for (var i = 0; i < orderIds.length; i++) {
		var id = orderIds[i];
		var info = orderInfo[i];
		var rangeI;
		if (info.type == 'vertex') {
			rangeI = settings.getVertex(id).getRange();
		} else if (info.type == 'edge') {
			rangeI = settings.getEdge(id).getRange();
		} else if (info.type == 'face') {
			rangeI = settings.getFace(id).getRange(info.vertexId);
		}
		range = range.intersect(rangeI);
	}
	return range;
};

ms.netTransistor.prototype.setPlacements = function(orderIds, orderInfo) {	
	var settings = this.settings;
	for (var i = 0; i < orderIds.length; i++) {
		var id = orderIds[i];
		var info = orderInfo[i];
		if (info.type == 'vertex') {
			settings.getVertex(id).setPosition();
		} else if (info.type == 'face') {
			settings.getFace(id).setFromVertex(info.vertexId);
		}
	}
};

ms.netTransistor.prototype.sampleFaceCentric = function() {
	var settings = this.settings;
	var { basisIds, lower, upper } = settings;
	// Create the four points on the ground.
	var self = this;
	if (this.ground) {
		var result = [];
		var vertices = this.endNet.interior.vertices;
		if (vertices.length == 4) {
			var result = [];
			vertices.forEach((vertex) => {
				if (self.dims == 3) {
					if (vertex.halfEdges[0].getDir().x > 0.9) {          // +X
						result = result.concat([[lower[0]], [lower[1]], [lower[2] + 1]]);
					} else if (vertex.halfEdges[0].getDir().x < -0.9) {  // -X
						result = result.concat([[upper[0]], [upper[1]], [lower[2] + 1]]);
					} else if (vertex.halfEdges[0].getDir().y > 0.9) {  // +Y
						result = result.concat([[upper[0]], [lower[1]], [lower[2] + 1]]);
					} else if (vertex.halfEdges[0].getDir().y < -0.9) {  // -Y
						result = result.concat([[lower[0]], [upper[1]], [lower[2] + 1]]);
					}
				} else {
					var signX = 0;
					var signY = 0;
					if (vertex.halfEdges[0].getDir().x >  0.9 || vertex.halfEdges[1].getDir().x >  0.9) { signX =  1; }
					if (vertex.halfEdges[0].getDir().x < -0.9 || vertex.halfEdges[1].getDir().x < -0.9) { signX = -1; }
					if (vertex.halfEdges[0].getDir().y >  0.9 || vertex.halfEdges[1].getDir().y >  0.9) { signY =  1; }
					if (vertex.halfEdges[0].getDir().y < -0.9 || vertex.halfEdges[1].getDir().y < -0.9) { signY = -1; }
					if (signX == 0 || signY == 0) {
						ms.alert('2D boundary corner is wrong.');
					}
					var xPos = (signX == 1) ? lower[0] + 0.1 : upper[0] - 0.1;
					var yPos = (signY == 1) ? lower[1] + 0.1 : upper[1] - 0.1;
					result = result.concat([[xPos], [yPos]]);
				}
			});
			return result;
		};
	}

	var success = true;
	this.fixedFaces.forEach((fixed) => {
		fixed.fPlace.setD(fixed.d);
		fixed.fPlace.setFixed(true);
	});
	this.fixedVertexIds.forEach((id) => {
		success = success && settings.getVertex(id).fixPosition();
	});
	if (!success) {
		return null;
	}

	var basisOrders = [];
	for (var i = 0; i < basisIds.length; i++) {
		basisOrder = settings.orderIds.findIndex((id, index) => {
			return id == basisIds[i] && settings.orderInfo[index].type == 'face';
		});
		basisOrders.push(basisOrder);
	}
	
	for (var i = 0; i < basisIds.length; i++) {
		var id = basisIds[i];
		var fPlace = settings.facePlacements[id];
		var start = basisOrders[i];
		var end = basisOrders[i + 1];		
		var orderIds  = settings.orderIds. slice(start, end);
		var orderInfo = settings.orderInfo.slice(start, end);
		var range = this.getRange(orderIds, orderInfo);

		if (fPlace.getFixed() && !range.isInside(fPlace.getD())) {
			this.effort = Infinity;
			return null;
		}
		if (range.isEmpty()) {
			return null;
		}
		if (!fPlace.getFixed()) {
			var d = range.sample();
			fPlace.setD(d);
		}
		orderIds.shift();
		orderInfo.shift();
		this.setPlacements(orderIds, orderInfo);
	};
	var positions = [];
	var numVertices = this.freeVertices.length;
	for (var i = 0; i < numVertices; i++) {
		var vertex = this.freeVertices[i];
		var id = vertex.getNode().getId();
		var position = settings.vertexPlacements[id].getPosition();
		for (var j = 0; j < this.dims; j++) {
			positions.push([position.getValue(j)]);
		}
	}
	
	return positions;
};

ms.netTransistor.prototype.sampleSolutionSpace = function(mutationArea) {
	ms.timerG.start('Sample Solutions');	
	var edgeCount = this.graph.edges.length;
	var vertexCols = this.dims * this.freeVertices.length;
	
	this.effort = 0;
	var limits = this.findLimits(mutationArea);

	while (true) {
		if (this.effort > ms.netTransistor.maxEffort) {
			ms.timerG.stop('Sample Solutions');
			return false;
		}
		var positions = this.sampleFaceCentric();
		var hasViolations = !positions || this.hasViolations(positions, limits);
		if (!hasViolations) {
			ms.timerG.stop('Sample Solutions');
			ms.timerG.start('Place Vertices');
			if (this.placeVertexPositions(positions)) {
				ms.timerG.stop('Place Vertices');
				return true;
			}
			ms.timerG.stop('Place Vertices');
			ms.timerG.start('Sample Solutions');
		}
		this.effort++;
	}
};

ms.netTransistor.prototype.getFreeablePaths = function() {
	return this.openPaths.filter(function(path) {
		return path.endpoints[0] !== path.endpoints[1] && path.extendableness() > 0;
	});
};

// Frees a vertex and any vertices that are attached by a rigid transformation.
ms.netTransistor.prototype.freeVertex = function() {
	// Filter out any paths that would create a completely disconnected component,
	// not fixed to anything.
	var freeablePaths = this.getFreeablePaths();
	// Return if there are no vertices to free.
	if (freeablePaths.length == 0) {
		this.effort++;
		return;
	}
	
	// Pick a random open paths.
	var path = ms.pick(freeablePaths);
	var vertex = path.randomNextVertex();
	this.freeOneVertex(vertex);

	var done = false;
	while (!done) {
		done = true;
		freeablePaths = this.getFreeablePaths();
		for (var i = 0; i < freeablePaths.length; i++) {
			var rigidVertex = freeablePaths[i].rigidNextVertex();
			if (rigidVertex) {				
				// debugging.
				// var rigidVertex = freeablePaths[i].rigidNextVertex();
				this.freeOneVertex(rigidVertex);
				done = false;
				break;
			}
		}
	}
};

// Frees an individual vertex.
ms.netTransistor.prototype.freeOneVertex = function(vertex) {
	var extents = this.model.getExtents();
	// var debugCell = this.model.getCell(extents[0] - 1, extents[1] - 1);
	var vertexEndpoints = vertex.getEndpoints();
	for (var i = 0; i < vertexEndpoints.length; i++) {
		var vEndpoint = vertexEndpoints[i];
		var line = vEndpoint.getLine();
		var hasLine = function(e) { return e.line == line; };
		if (!this.lineData.find(hasLine)) {
			this.addLine(line, true, true);
			var lineState = line.getSegment().getStates()[0];
			lineState.removeCells();
			// debugCell.addState(lineState);
		}
	}
	var self = this;
	for (var i = 0; i < vertexEndpoints.length; i++) {
		var vEndpoint = vertexEndpoints[i];
		var line = vEndpoint.getLine();
		
		var expandFunc = function(path, goForward) {
			if (goForward) {
				path.expandForward();
			} else {
				path.expandBackward();
			}
		};
		var path0 = this.openPaths.find(function(path) {
			return path.endpoints[0] == vEndpoint;
		});
		var path1 = this.openPaths.find(function(path) {
			return path.endpoints[1] == vEndpoint;
		});
		var path;
		if (path0 && path1) {
			if (path0 == path1) {
				path = path0;
				path.endpoints = [];
				ms.remove(path, this.openPaths);
			} else {
				path = path1;
				path.merge(path0);
				ms.remove(path0, this.openPaths);
			}
		} else if (path0) {
			path = path0;
			expandFunc(path, false);
		} else if (path1) {
			path = path1;
			expandFunc(path, true);
		} else {
			path = new ms.transistorPath([], this.lines);
			path.setEndpoints([vEndpoint, vEndpoint]);
			path.expandBackward();
			path.expandForward();
			this.openPaths.push(path);
		}
	}
};

ms.netTransistor.prototype.placeVertexPositions = function(positions) {
	var freeVertices = this.freeVertices;
	for (var i = 0; i < freeVertices.length; i++) {
		var position = this.dims == 2 ?
			new ms.vec2(positions[this.dims * i][0], positions[this.dims * i + 1][0]) :
			new ms.vec3(positions[this.dims * i][0], positions[this.dims * i + 1][0], positions[this.dims * i + 2][0]);
		freeVertices[i].setPosition(position);
		if (!this.model.inBounds(position.x, position.y, position.z)) {
			return false;
		}
	}	
	
	// Faces to update the face connection.
	var facesToUpdate = new Set();

	// 2D Edge Intersections. 3D is handled below.
	if (this.dims == 2) {
		for (var i = 0; i < this.propagationOrder.length; i++) {
			var endpoint = this.propagationOrder[i];
			var line = endpoint.getLine();
			line.moveToEndpoints();
			var lineState = line.getSegment().getStates()[0];
			var intersection = lineState.addToModelWithIntersections(-Infinity);
			if (intersection) {
				if (!intersection.state) {
					return false;
				}
				// Reset the line states, remove them from the model.
				for (var j = 0; j <= i; j++) {				
					var endpoint = this.propagationOrder[j];
					var line = endpoint.getLine();
					var lineState = line.getSegment().getStates()[0];
					lineState.removeCells();
				}
				return false;
			}
			var intersections = lineState.findConnectionIntersections();
			intersections.forEach(function(connection) {
				facesToUpdate.add(connection.getFace());
			});
		}
	}	
	
	var coreVertices1 = this.openPaths.map(function(path) { return path.endpoints[0].getVertex(); });
	var coreVertices2 = freeVertices;
	// var coreVertices3 = this.graph.getVertices().filter(function(v) { return v.isBoundary(); }).map(getCore);
	var coreVertices = coreVertices1.concat(coreVertices2);

	if (this.dims == 2) {
		// Find all the faces that were involved.
		var addFaces = function(endpoint) {
			facesToUpdate.add(endpoint.getFace());
			endpoint.getConnections().forEach(function(connection) {
				facesToUpdate.add(connection.getFace());
			});
		}
		this.freeEdges.forEach(function(edge) {
			var endpoints = edge.getEndpoints();
			addFaces(endpoints[0]);
			addFaces(endpoints[1]);
		});
		var success = true;
		facesToUpdate.forEach(function(face) {
			success = success && face.updateConnection();
		});
		if (!success) {
			return false;
		}
		
		this.freeEdges.forEach(function(edge) {
			var brush = edge.getBrush();
			var tiled = brush ? brush.get('Tiled') : true;
			if (tiled && brush) {
				var endpoints = edge.getEndpoints();
				var length = endpoints[0].getPosition().distance(endpoints[1].getPosition());
				var lengthInTiles = length / brush.get('Tile Length');
				var numTiles = Math.max(1, Math.round(lengthInTiles));
				edge.getState().getCoordinates().angledEdges[1].t = numTiles;
			}
		});
	} else {
		var facesToIntersect = Object.values(this.settings.facePlacements).map((fPlace) => fPlace.face);
		// There are face placements with no faces for the leaf nodes and single faced edges.
		facesToIntersect = facesToIntersect.filter((face) => !!face);
		
		var success = facesToIntersect.every((faceA) => {
			var faces = faceA.getGroup().getFaces();
			if (faces.length > 1) {
				if (faceA.isHole()) {
					if (!faces[0].containsFace(faceA)) {
						return false;
					}
					if (ms.globalSettings.get('Cut Holes')) {
						return faces.every((faceB, index) => {
							return faceA == faceB || (index == 0) || faceA.outsideFace(faceB);
						});
					}
					return true;
				} else {
					// If faceA is not a hole every face in the group must be inside of it.
					return faces.every((faceB) => {
						return faceA == faceB || faceA.containsFace(faceB);
					});
				}
			}
			return true;
		});
		if (!success) {
			return false;
		}
		var tree = this.model.getBspTree();
		for (var i = 0; i < facesToIntersect.length; i++) {
			var face = facesToIntersect[i]; 
			var plane = ms.bspPlane.create(face);
			var polygon = ms.bspPolygon.create(face);
			// For ground the endpoints, may be out of order, so skip selfIntersects.
			var success = this.ground || !polygon.selfIntersects();
			success = success && tree.add(plane, polygon);
			if (!success) {				
				for (j = 0; j <= i; j++) {
					var face = facesToIntersect[j];
					while(face.getPolygons().length > 0) {
						face.getPolygons()[0].getNode().destroy();
					}
				}
				return false;
			}
		}
	}
	return true;
};

ms.netTransistor.prototype.print = function() {
	// Not sure why we need to use vertices here.
	this.graph.vertices[0].print();
};

ms.netTransistor.prototype.reject = function() {
	if (!this.graph) {
		return;
	}
	// Set the cost really high.
	this.stats.costChange['reject'] = 1e6;
};
