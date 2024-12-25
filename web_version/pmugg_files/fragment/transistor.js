ms.transistor = function() {};

// A global variable for handling matrix math.
var mathG;

ms.transistor.MAX_ANGLE_DIFFERENCE = 45 / 180 * Math.PI;

ms.transistor.buildNormally = function(transition) {
	var result = new ms.transistor();
	result.create(transition);
	if (result.effort > 0) {
		result.reject();
		return null;
	} else {
		return result;
	}
};

ms.transistor.prototype.create = function(transition) {
	this.startInstance = transition.startInstance;
	this.endGroup = transition.endGroup;
	
	this.openPaths = [];
	this.lineData = [];
	this.lines = [];
	this.stats = this.startInstance.getStats();
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

	var endpointAngles = [];
	ms.timerG.start('Create Graph');
	var success = this.mergeDuplicateLines();
	if (!success) {
		this.effort = Infinity;
		return;
	}
	this.graph = this.createGraph(endpointAngles);
	ms.timerG.stop('Create Graph');
	if (!this.graph) {
		this.effort = Infinity;
		return;
	}
	this.dims = this.graph.getLink('is3D') ? 3 : 2;

	var edges = this.graph.getEdges();
	for (var i = 0; i < edges.length; i++) {
		this.addLine(edges[i].getCore(), false, false);
	}
	this.startInstance.destroy();
};

ms.transistor.pickAngle = function(angles) {
	if (!ms.globalSettings.get('Allow Rotations')) {
		return 0;
	}
	if (angles.length == 0) {
		return ms.randomUniform(-Math.PI / 2, Math.PI / 2);
	}
	var min = Infinity;
	var max = -Infinity;
	for (var i = 0; i < angles.length; i++) {
		min = Math.min(min, angles[i]);
		max = Math.max(max, angles[i]);
	}
	if (max - min > ms.transistor.MAX_ANGLE_DIFFERENCE) {
		return 1e9;
	} else {
		return (min + max) / 2;
	}
};

ms.transistor.maxEffort = 10;
ms.transistor.precision = 1e-8;
ms.transistor.constraintPrecision = 1e-5;
ms.transistor.minLength = 0.005;
ms.transistor.defaultLengthMin = 0.1;
ms.transistor.defaultLengthMax = 4;

// This is useful for debugging, so we can see the states when they are first created.
ms.transistor.addToModelInitially = false;

ms.transistor.prototype.addLine = function(line, includeLength, addToGraph) {	
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

	if (addToGraph) {
		var vertices = this.graph.getVertices();
		var lineVertices = lineEndpoints.map(function(endpoint) {
			var vertexCore = endpoint.getVertex();
			var result = ms.transistor.findVertexWithCore(vertexCore, vertices);
			if (!result) {
				result = new ms.graphVertex(vertexCore);
			}
			return result;
		});
		this.graph.addEdge(new ms.graphEdge(line), lineVertices[0], lineVertices[1]);
	}
};

ms.transistor.findVertexWithCore = function(coreVertexA, verticesB) {
	return verticesB.find(function(vertexB) {
		return vertexB.getCore() == coreVertexA;
	});
};

// True if the edge can be extended.
ms.transistor.isExtendable = function(edge) {
	return edge.getCore().getEdgeType().extendable();
};

// Merge two endpoints at the given indices.
ms.transistor.prototype.mergeEndpoints = function(lineIndices, endpointIndices) {
	var newEndGroup = this.endGroup.mergeEndpoints(endpointIndices);
	if (!newEndGroup) { return false;}
	this.endGroup = newEndGroup;
	this.startInstance = this.startInstance.mergeEndpoints(lineIndices, endpointIndices);
	return true;
};

ms.transistor.prototype.mergeDuplicateLines = function() {
	// For a given line where does it show up in start lines.
	var lineIdToIndices = {};
	var startLines = this.startInstance.getLines();
	for (var i = 0; i < startLines.length; i++) {
		var id = startLines[i].getNode().getId();
		if (!lineIdToIndices[id]) {
			lineIdToIndices[id] = [];
		}
		lineIdToIndices[id].push(i);
	}
	var startGroup = this.startInstance.getGroup();
	var duplicateIndices = null;
	Object.values(lineIdToIndices).forEach(function(lineIndices) {
		if (lineIndices.length > 2) {
			ms.alert('More than two endpoints share the same line.')
		}
		if (lineIndices.length == 2) {
			duplicateIndices = lineIndices;
		}
	});
	if (duplicateIndices) {
		var endpointIndex0 = startGroup.getEndpointData().findIndex(function(datum) { return datum.lineIndex == duplicateIndices[0]; });
		var endpointIndex1 = startGroup.getEndpointData().findIndex(function(datum) { return datum.lineIndex == duplicateIndices[1]; });
		
		var success = this.mergeEndpoints(duplicateIndices, [endpointIndex0, endpointIndex1]);
		if (!success) { return false; }
		this.mergeDuplicateLines();
	}
	return true;
}

ms.transistor.prototype.createGraph = function(endpointAngles) {
	var endGraph = this.endGroup.getGraph();
	var mergedGraph = endGraph.copy();
	var mergedVertices = mergedGraph.getVertices();
	var mergedEdges = mergedGraph.getEdges().slice();
	var startGroup = this.startInstance.getGroup();

	var splitLines = this.startInstance.getLines().slice().map(function(line) { return [line]});
	if (ms.globalSettings.get('Allow Rotations')) {
		for (var i = 0; i < mergedEdges.length; i++) {
			var mergedEdge = mergedEdges[i];
			for (var e = 0; e < 2; e++) {
				var mergeEndpoint = mergedEdge.getEndpoint(e);
				var mergeIndex = mergedGraph.getOuterEndpoints().indexOf(mergeEndpoint);
				// Add the angles for the outer endpoints. (I think).
				if (mergeIndex >= 0) {
					var endIndex = this.endGroup.getOuterEndpoints().indexOf(endGraph.getOuterEndpoints()[mergeIndex]);
					var startIndex = startGroup.endpointToLineIndex(endIndex);
					// The lines are not split yet.
					var coreEndpoint = splitLines[startIndex][0].getEndpoints()[e];
					endpointAngles.push(coreEndpoint.angleOffset());
				}
			}
		}
	}
	this.angle = ms.transistor.pickAngle(endpointAngles);
	if (this.angle > 100) {
		return null;
	}
	
	this.dims = mergedGraph.getLink('is3D') ? 3 : 2;
	for (var i = 0; i < mergedVertices.length; i++) {
		var v = mergedVertices[i];
		if (!v.isOuter()) {
			var type = v.getCore();
			var randomPosition = this.dims == 2 ?
				new ms.vec2(5 * Math.random(), 5 * Math.random()) :
				new ms.vec3(5 * Math.random(), 5 * Math.random(), 5 * Math.random());
			v.setCore(ms.vertex.createWithState(this.stats, randomPosition, this.angle, 1, type));
		}
	}
	ms.taskDebug();
	
	var edgeData = [];
	// A mapping from the core vertex to the graph vertices. Just for the outer vertices.
	var coreToGraphV = {};
	for (var i = 0; i < mergedEdges.length; i++) {
		var mergedEdge = mergedEdges[i];
		var coreEndpointsI = [];
		var graphVerticesI = [];
		var modified = false;
		for (var e = 0; e < 2; e++) {
			var mergeEndpoint = mergedEdge.getEndpoint(e);
			var mergeIndex = mergedGraph.getOuterEndpoints().indexOf(mergeEndpoint);
			// For each outer endpoint find the matching endpoint in the original graph (in the instance).
			// For the other endpoints, just save the data we have for them.
			if (mergeIndex >= 0) {
				var endIndex = this.endGroup.getOuterEndpoints().indexOf(endGraph.getOuterEndpoints()[mergeIndex]);
				var startIndex = startGroup.endpointToLineIndex(endIndex);
				if (splitLines[startIndex].length == 1) {
					if (splitLines[startIndex][0].getNode().isDestroyed()) {
						ms.alert('The same line is being split twice. This should not happen. Exiting early may not work.');
						return null;
					}
					splitLines[startIndex] = splitLines[startIndex][0].split().lines;
				}
				var coreEndpoint = splitLines[startIndex][e].getEndpoints()[e];
				splitLines[startIndex][e] = null;
				coreEndpointsI[e] = coreEndpoint;

				// Check if the graph vertex has already been created.
				// This happens when two of the outer vertices match.
				var coreVertex = coreEndpoint.getVertex();
				var coreId = coreVertex.getNode().getId();
				var graphVertex = coreToGraphV[coreId];
				if (!graphVertex) {
					graphVertex = new ms.graphVertex(coreVertex);
					coreToGraphV[coreId] = graphVertex;
				}				
				graphVerticesI.push(graphVertex);
				modified = true;
			} else {
				var mergeEndpoint = mergedEdge.getEndpoint(e);
				var graphVertex = mergeEndpoint.getVertex();

				var coreEndpoint = graphVertex.getCore().getState().getEndpoints()[mergeEndpoint.getVertexIndex()];
				coreEndpointsI.push(coreEndpoint);
				graphVerticesI.push(graphVertex);
			}
		}
		edgeData.push({coreEndpoints: coreEndpointsI, graphVertices: graphVerticesI, modified: modified});
	}
	
	var extents = this.model.getExtents();
	var debugCell = this.model.getCell(extents[0] - 1, extents[1] - 1);
	for (var i = 0; i < edgeData.length; i++) {
		var mergedEdge = mergedEdges[i];
		var datum = edgeData[i];
		var endpoint0 = datum.coreEndpoints[0];
		var endpoint1 = datum.coreEndpoints[1];
		var line0 = endpoint0.getLine();
		var line1 = endpoint1.getLine();
		line0.addEndpoint(endpoint1, line0.getEndpoints()[1] ? 0 : 1);
		line0.fillFromEndpoints(ms.transistor.addToModelInitially);
		debugCell.addState(line0.getSegment().getStates()[0]);
		line1.getNode().destroy();
		mergedEdge.setCore(line0);

		if (datum.modified) {			
			mergedGraph.removeEdge(mergedEdge);
			mergedGraph.addEdge(mergedEdge, datum.graphVertices[0], datum.graphVertices[1]);
		}
	}
	
	var endpointEndToMerged = function(endEndpoint) {
		var graphEdge = mergedEdges[endEdges.indexOf(endEndpoint.getEdge())];
		var graphEndpoint = new ms.graphEndpoint(null, graphEdge);
		graphEndpoint.setEdgeIndex(endEndpoint.getEdgeIndex());
		return graphEndpoint;
	};
	
	var endOuterEndpoints = this.endGroup.getOuterEndpoints().slice();
	var endEdges = endGraph.getEdges();
	var mergedEdgeNewOrder = mergedGraph.getEdges();  // Not the same as mergedEdges.
	for (var i = 0; i < endOuterEndpoints.length; i++) {
		var faceEndpointsI = ms.graph.faceEndpoints(endOuterEndpoints[i]).slice();
		var pathFaceEndpoints = faceEndpointsI.map(endpointEndToMerged);

		var path = ms.transistorPath.create(pathFaceEndpoints, mergedEdgeNewOrder, this.lines);
		var pathEndpoints = [
			pathFaceEndpoints[0].getCore(),
			pathFaceEndpoints[pathFaceEndpoints.length - 1].twin().getCore().clockwise(),
		];
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
		return mergedGraph;
	} else {
		return null;
	}
};

ms.transistor.prototype.solve = function(mutationArea) {
	var success = this.setup(mutationArea);
	if (this.changeBasis) {
		return this.sampleSolutionSpace(mutationArea);
	} else {
		return success;
	}
};

ms.transistor.prototype.setup = function(mutationArea) {
	if (ms.globalSettings.get('Fast Matrix Math')) {
		mathG = ms.fastMath;
	} else {
		mathG = math;
	}
	
	ms.timerG.start('Setup');
	var success = success = this.setupCore(mutationArea);
	ms.timerG.stop('Setup');
	return success;
};

ms.transistor.setBoundaryVertexPosition = function(vertex, mutationArea) {
	if (!(vertex.getCore() instanceof ms.vertex)) {
		// This can happen when drawing the graph during debugging.
		return;
	}
	var x = -1;
	var y = -1;
	vertex.getEndpoints().forEach(function(endpoint) {
		var rounded = Math.round(endpoint.getAngle() / (Math.PI /2));
		rounded = (rounded + 4) % 4;
		if (rounded == 0) { x = 0; }
		if (rounded == 1) { y = 0; }
		if (rounded == 2) { x = 1; }
		if (rounded == 3) { y = 1; }
	});
	x = (x == 0) ? mutationArea.lowerExtent[0] + 0.1 : mutationArea.upperExtent[0] - 0.1;
	y = (y == 0) ? mutationArea.lowerExtent[1] + 0.1 : mutationArea.upperExtent[1] - 0.1;
	vertex.getCore().setPosition(new ms.vec2(x, y));
};

ms.transistor.origin = function(dims) {
	return (dims == 2) ? ms.vec2.ORIGIN : ms.vec3.ORIGIN;
};

ms.transistor.prototype.setupCore = function(mutationArea) {
	this.changeBasis = null;
	this.isDiscretized = null;
	this.edgeBlockers = [];
	this.mutationArea = mutationArea;
	
	this.edgeData = {};
	this.freeVertices = [];
	
	this.freeEdges = this.graph.edges.filter(function(edge) {
		return ms.transistor.isExtendable(edge);
	});
	var rigidEdges = this.graph.edges.filter(function(edge) { return !ms.transistor.isExtendable(edge); });

	var constrainedVertices = {};
	var nonOuter = this.graph.vertices.find(function(v) { return !v.isOuter() });
	if (nonOuter && nonOuter.getCore().getNode) {
		var getId = function(v) {
			return v.getCore().getNode().getId();
		}
	} else {
		// This is just need for displaying pretty graphs.
		var getId = function(v) {
			return v.getCore().id;
		}
	}
	// Vertices that have exactly one constraint.
	var singleConstraintList = [];
	var getConstrained = function(v) {
		return constrainedVertices[getId(v)];
	}
	var dims = this.dims;
	var constraintSettings = {
		goalGraph: this.endGroup.graph,
		getConstrained: getConstrained,
		singleConstraintList: singleConstraintList,
		freeVertices: this.freeVertices,
		angle: this.angle,
		dims: dims,
	};
	var self = this;
	this.graph.vertices.forEach(function(v) {
		if (!v.isOuter()) {
			var id = getId(v);
			self.freeVertices.push(v);
			constrainedVertices[id] = new ms.constrainedVertex(v, constraintSettings);
		}
	});
	rigidEdges.forEach(function(edge) {
		var vertex0 = getConstrained(edge.getEndpoint(0).getVertex());
		var vertex1 = getConstrained(edge.getEndpoint(1).getVertex());
		var offset = edge.getCore().getEdgeType().getOffset();
		vertex0.setRigidConstraint(vertex1, offset.copy());
	});
	
	var freeEdgeCount = this.freeEdges.length;

	var vertexRows = dims * this.freeVertices.length;
	var numRows = vertexRows + freeEdgeCount;
	var getEdgeRow = function(e) {
		return self.freeEdges.indexOf(e) + dims * self.freeVertices.length;
	}
	var rowToEdge = function(row) {
		return self.freeEdges[row - dims * self.freeVertices.length];
	}
	constraintSettings.getEdgeRow = getEdgeRow;
	// The vertex positions and edge lengths are given by mx + b where x is a vector of free edge lengths.
	constraintSettings.M = mathG.zeros(numRows, 0);
	constraintSettings.b = mathG.zeros(numRows, 1);
	constraintSettings.edgeRows = [];

	var updateRows = function() {
		self.freeVertices.forEach(function(v, index) {
			getConstrained(v).setRow(dims * index);
		});
	};
	updateRows();
	
	var removeRow = function(row) {
		var M = constraintSettings.M;
		var b = constraintSettings.b;
		var mSize = M.size();
		var rowIndices = ms.util.sequence(0, mSize[0] - 1);
		rowIndices.splice(row, dims);
		if (mSize[1] > 0) {
			constraintSettings.M = mathG.subset(M, mathG.index(rowIndices, mathG.range(0, mSize[1])));
		} else {
			constraintSettings.M = mathG.zeros(rowIndices.length, 0);
		}
		constraintSettings.b = mathG.subset(b, mathG.index(rowIndices, [0]));
	}
	var removeVertex = function(v) {
		ms.remove(v.vertex, self.freeVertices);
		removeRow(v.row);
		updateRows();
	};
	var fixVertex = function(vertexToFix) {
		var pos = vertexToFix.getCore().getPosition();
		var constrainedToFix = getConstrained(vertexToFix);
		removeVertex(constrainedToFix);
		return constrainedToFix.fixVertex(pos.toMatrix());
	};

	for (var i = 0; i < this.openPaths.length; i++) {
		var path = this.openPaths[i];
		for (var j = 0; j < 2; j++) {
			var pathVertex = path.endpoints[j].getVertex();
			var vertexToFix = this.freeVertices.find(function(freeVertex) {
				return freeVertex.getCore() == pathVertex;
			});
			if (vertexToFix) {
				var success = fixVertex(vertexToFix);
				if (!success) { return false; }
			}
		}
	}
	var prevVertices = this.freeVertices.slice();
	for (var i = 0; i < prevVertices.length; i++) {
		var vertex = prevVertices[i];
		if (vertex.isBoundary() && (vertex.getCore() instanceof ms.vertex)) {
			ms.transistor.setBoundaryVertexPosition(vertex, mutationArea);
			var success = fixVertex(vertex);
			if (!success) { return false; }
		}
	}

	var fixedEdges = this.freeEdges.filter(function(edge) {
		return !self.freeVertices.includes(edge.getEndpoint(0).getVertex()) &&
		       !self.freeVertices.includes(edge.getEndpoint(1).getVertex());
	});
	fixedEdges.forEach(function(edge) {
		var constraint = {endpoint: edge.getEndpoint(1), offset: ms.transistor.origin(dims)};
		var constrained = getConstrained(edge.getEndpoint(0).getVertex());
		constrained && constrained.updateEdgeLength(constraint);
	});
	
	vertexRows = 2 * this.freeVertices.length;
	numRows = vertexRows + freeEdgeCount;
	if (numRows == 0) {
		return true;
	}

	// If none of the vertices are fixed, fix the first vertex to a random position.
	var randomOffset = [];
	if (this.openPaths.length == 0 && this.freeVertices.length > 0) {
		var randomX = ms.randomUniform(mutationArea.lowerExtent[0], mutationArea.upperExtent[0]);
		var randomY = ms.randomUniform(mutationArea.lowerExtent[1], mutationArea.upperExtent[1]);
		var randomZ = ms.randomUniform(mutationArea.lowerExtent[2], mutationArea.upperExtent[2]);
		var pos = (dims == 2) ? [[randomX], [randomY]] : [[randomX], [randomY], [randomZ]]
		var success = getConstrained(this.freeVertices[0]).fixVertexSoft(mathG.matrix(pos));
		if (!success) { return false; }
	}

	while (singleConstraintList.length > 0) {
		var success = singleConstraintList[0].applySingle();
		if (!success) { return false; }
	}
	// I thought this might be needed, but it's not.
	// constraintSettings.linearConstraints.forEach(function(c) { c.vertex.updateEdgeLength(c.constraint);	});

	this.changeBasis = constraintSettings.M;
	this.initialPosition = constraintSettings.b;
	this.isDiscretized = ms.transistor.discretizeBasis(this.changeBasis, this.freeEdges);
	this.basisEdges = constraintSettings.edgeRows.map(rowToEdge);

	if (ms.globalSettings.get('Bend Lines')) {
		this.findPropagationOrder();
	} else {
		this.propagationOrder = this.graph.edges.map(function(edge) { return edge.getEndpoint(0); });
	}
	return true;
};

ms.transistor.roundingTolerance = 1e-5;
ms.transistor.maxDiscreteLength = 10;

ms.transistor.enableDiscretization = true;

ms.transistor.discretizeBasis = function(changeBasis, freeEdges) {
	var edges = freeEdges;
	var size = changeBasis.size();
	var numVertices = size[0] - edges.length;
	var isDiscretized = [];
	
	if (!ms.transistor.enableDiscretization) {		
		for (var col = 0; col < size[1]; col++) {
			isDiscretized.push(false);
		}
	}

	for (var col = 0; col < size[1]; col++) {
		var discretized = [];
		var maxBasis = 0;
		for (var row = 0; row < edges.length; row++) {
			var length = changeBasis.get([numVertices + row, col]);
			discretized[row] = length / edges[row].getCore().getEdgeType().getEdgeLength();
			maxBasis = Math.max(length, maxBasis);
		}
		var minLength = Infinity;
		var minIndex = -1;
		for (var row = 0; row < edges.length; row++) {
			if (discretized[row] > ms.transistor.roundingTolerance && discretized[row] < minLength) {
				minLength = discretized[row];
				minIndex = row;
			}
		}
		if (minIndex == -1) {
			isDiscretized.push(false);
		} else {
			var discreteLength = 1 / minLength;
			var minBasis = changeBasis.get([numVertices + minIndex, col]);
			var maxIterations = ms.transistor.maxDiscreteLength / (discreteLength * maxBasis / minBasis);
			var acceptable = true;
			for (var i = 1; i < maxIterations; i++) {
				acceptable = true;
				for (var row = 0; row < edges.length; row++) {
					var length = discretized[row] * i * discreteLength;
					if (Math.abs(length - Math.round(length)) > ms.transistor.roundingTolerance) {
						acceptable = false;
						break;
					}
				}
				if (acceptable) {
					var scale = i * discreteLength;
					for (var row = 0; row < size[0]; row++) {
						changeBasis.set([row, col], scale * changeBasis.get([row, col]));
					}
					isDiscretized.push(true);
					break;
				}
			}
			if (!acceptable) {
				ms.alert('No discrete solution found.');
			}
		}
	}
	return isDiscretized;
}

ms.transistor.prototype.findLimits = function(mutationArea) {
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
		var edgeType = this.freeEdges[i].getCore().getEdgeType();
		var brush = edgeType.getBrush();
		var minLength = ms.transistor.minLength;
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

ms.transistor.prototype.hasViolations = function(positions, samples, limits) {
	for (var i = 0; i < this.edgeBlockers.length; i++) {
		if (this.edgeBlockers[i].isBlocking(samples)) {
			return true;
		}
	}
	for (var i = 0; i < positions.length; i++) {
		var value = positions[i][0];
		if (value < limits.min[i] || value > limits.max[i]) {
			return true;
		}
	}
	return false;
};

ms.transistor.prototype.sampleSolutionSpace = function(mutationArea) {
	ms.timerG.start('Sample Solutions');	
	var edgeCount = this.graph.edges.length;
	var vertexCols = this.dims * this.freeVertices.length;
	
	var size = this.changeBasis.size();
	this.effort = 0;
	var limits = this.findLimits(mutationArea);

	while (true) {
		if (this.effort > ms.transistor.maxEffort) {
			ms.timerG.stop('Sample Solutions');
			return false;
		}

		var multiplier = 1;
		var multiplierBrush = null;
		for (var i = 0; i < size[1]; i++) {
			var sample;
			var edge = this.basisEdges[i];
			var brush = edge && edge.getCore().getEdgeType().getBrush();			
			if (brush && brush.get('Power Law')) {
				var p = brush.get('Power');
				var r = ms.randomUniform(0, 1);
				// multiplier = Math.pow(1 - r, -1 / (p - 1));
				multiplier = Math.pow(r, p);
				multiplierBrush = brush;
			}
		}

		// Find the maximum possible sample values.
		var sampleMaxs = (new Array(size[1])).fill(Infinity);
		for (var i = 0; i < size[0]; i++) {
			var hasPositive = false;
			var hasNegative = false;
			for (var j = 0; j < size[1]; j++) {
				var value = this.changeBasis.get([i, j]);
				if (value > 1e-4) {
					hasPositive = true;
				} else if (value < -1e-4) {
					hasNegative = true;
				}
			}
			if (hasNegative && !hasPositive) {
				var b = this.initialPosition.get([i, 0]);
				for (var j = 0; j < size[1]; j++) {
					var change = this.changeBasis.get([i, j]);
					if (change < 0) {
						sampleMaxs[j] = Math.min(sampleMaxs[j], (limits.min[i] - b) / change);
					}
				}
			}
			if (!hasNegative && hasPositive) {
				var b = this.initialPosition.get([i, 0]);
				for (var j = 0; j < size[1]; j++) {
					var change = this.changeBasis.get([i, j]);
					if (change > 0) {
						sampleMaxs[j] = Math.min(sampleMaxs[j], (limits.max[i] - b) / change);
					}
				}
			}
		}
		
		var samples = [];
		for (var i = 0; i < size[1]; i++) {
			var sample;
			var edge = this.basisEdges[i];
			var brush = edge && edge.getCore().getEdgeType().getBrush();
			var lengthMin = brush ? brush.get('Min Length') : ms.transistor.defaultLengthMin;
			var lengthMax = brush ? brush.get('Max Length') : ms.transistor.defaultLengthMax;
			lengthMax = Math.min(lengthMax, sampleMaxs[i]);
			if (lengthMin > lengthMax) {
				return false;
			}
			if (this.isDiscretized[i]) {
				var minValue = Math.ceil(lengthMin);
				var maxValue = Math.floor(lengthMax);
				if (maxValue < minValue) {
					ms.alert('Problem in discretization.');
				}
				sample = minValue + ms.random(maxValue - minValue + 1);
			} else {
				if (brush && brush == multiplierBrush) {
					sample = multiplier * ms.randomUniform(lengthMin, lengthMax);
					// This does the power law separate for each edge.
					/* var a = brush.get('Mean Length');
					var r = ms.randomUniform(0, 1);
					sample = lengthMin * Math.pow(1 - r, -1 / (a - 1)) */
				} else {
					sample = ms.randomUniform(lengthMin, lengthMax);
				}
			}
			samples.push([sample]);
			
		}
		samples = mathG.matrix(samples);
		var positions;
		if (size[1] > 0) {
		 	positions = mathG.add(mathG.multiply(this.changeBasis, samples), this.initialPosition).valueOf();
		} else {
			positions = this.initialPosition.valueOf();
			// The samples aren't changing.
			this.effort = Infinity;
		}
		var hasViolations = this.hasViolations(positions, samples, limits);
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

ms.transistor.prototype.getFreeablePaths = function() {
	return this.openPaths.filter(function(path) {
		return path.endpoints[0] !== path.endpoints[1] && path.extendableness() > 0;
	});
};

// Frees a vertex and any vertices that are attached by a rigid transformation.
ms.transistor.prototype.freeVertex = function() {
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
ms.transistor.prototype.freeOneVertex = function(vertex) {
	var extents = this.model.getExtents();
	var debugCell = this.model.getCell(extents[0] - 1, extents[1] - 1);
	var vertexEndpoints = vertex.getEndpoints();
	for (var i = 0; i < vertexEndpoints.length; i++) {
		var vEndpoint = vertexEndpoints[i];
		var line = vEndpoint.getLine();
		var hasLine = function(e) { return e.line == line; };
		if (!this.lineData.find(hasLine)) {
			this.addLine(line, true, true);
			var lineState = line.getSegment().getStates()[0];
			lineState.removeCells();
			debugCell.addState(lineState);
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

ms.transistor.prototype.findPropagationOrder = function() {
	var graph = this.graph;
	var vertices = graph.getVertices().filter(function(v) { return !v.isOuter(); });
	var edges = graph.getEdges();
	var vertexCount = 0;
	
	this.propagationOrder = [];
	if (graph.isEmpty()) {
		return;
	}
	var verticesToPropagate = [];	
	var isEdgeAdded = new Array(edges.length).fill(false);
	var isVertexAdded = new Array(vertices.length).fill(false);
	var startingPaths = this.openPaths.slice();
	while (vertexCount < vertices.length) {
		// First try all the open paths to start with.
		while (startingPaths.length > 0) {
			var startingPath = startingPaths.shift();
			var coreVertex = startingPath.endpoints[0].getVertex();
			var vertexIndex = vertices.findIndex(function(v) {
				return (v.getCore() == coreVertex);
			})
			if (!isVertexAdded[vertexIndex]) {
				verticesToPropagate.push(vertices[vertexIndex]);
				break;
			}
		}
		// If there are no open paths, start with the first vertex that hasn't been added.
		var i = 0;
		while (verticesToPropagate.length == 0) {
			if (!isVertexAdded[i]) {
				verticesToPropagate.push(vertices[i]);
			}
			i++;
		}
		while (verticesToPropagate.length > 0) {
			var vertex = verticesToPropagate.shift();
			var endpoints = vertex.getEndpoints();
			for (var i = 0; i < endpoints.length; i++) {		
				var endpoint = endpoints[i];
				var edgeIndex = edges.indexOf(endpoint.getEdge());
				var twinVertex = endpoint.twin().getVertex();
				var twinIndex = vertices.indexOf(twinVertex);
	
				// Check that the twin vertex hasn't already been added.
				if (!isVertexAdded[twinIndex]) {
					verticesToPropagate.push(twinVertex);
					isVertexAdded[twinIndex] = true;
					vertexCount++;
				}
				if (!isEdgeAdded[edgeIndex]) {
					this.propagationOrder.push(endpoint);
					isEdgeAdded[edgeIndex] = true;
				}
			}
		}
	}
};

ms.transistor.prototype.bendLines = function(positions) {
	var vertices = this.graph.getVertices();
	var straightToBentMap = new Array(vertices.length).fill(-1);
	var bentVertices = [];
	var linesToBentVertices = [];
	var freeCoreVertices = this.freeVertices.map(function(v) { return v.getCore(); });

	var lineLengths = [];
	for (var i = 0; i < this.propagationOrder.length; i++) {
		var endpointWrapper0 = this.propagationOrder[i];
		var endpoint0 = endpointWrapper0.getCore();
		var endpoint1 = endpointWrapper0.twin().getCore();
		var p0 = endpoint0.getPosition();
		var p1 = endpoint1.getPosition();
		lineLengths.push(p0.copy().minus(p1).length());
	}

	for (var i = 0; i < this.propagationOrder.length; i++) {
		var endpointWrapper0 = this.propagationOrder[i];
		var endpointWrapper1 = endpointWrapper0.twin();
		var endpoint0 = endpointWrapper0.getCore();
		var endpoint1 = endpointWrapper1.getCore();
		var line = endpoint0.getLine();
		var p0 = endpoint0.getPosition().copy();
		var p1 = endpoint1.getPosition().copy();
		var vertexIndex0 = vertices.indexOf(endpointWrapper0.getVertex());
		var vertexIndex1 = vertices.indexOf(endpointWrapper1.getVertex());
		var bentIndex0 = straightToBentMap[vertexIndex0];
		var bentIndex1 = straightToBentMap[vertexIndex1];
		if (bentIndex0 == -1 && bentIndex1 != -1) {
			ms.alert('Something is wrong with the bent index.');
		}

		var brush = line.getEdgeType().getBrush();
		var bendLength = (brush ? brush.get('Bend Length') : 0.1);
		var angle0 = endpoint0.getAngle();

		var fullLength = lineLengths[i];
		var numSegments = Math.max(Math.round(fullLength / bendLength), 1);
		var segmentLength = fullLength / numSegments;

		// bentId is unique for each vertex. The first time a vertex appears, the
		// bentId points the bentVertex to itself. When the same vertex appears in
		// a different bentVertex, this points to the first bentVertex.
		var bentId;
		if (bentIndex0 < 0) {
			straightToBentMap[vertexIndex0] = bentVertices.length;
			bentId = bentVertices.length;
		} else {
			bentId = bentIndex0;
		}
		
		var angleOffsets = ms.brush.bendSequence(brush, numSegments + 1);
		bentIndex0 = bentVertices.length;
		var angleOffset = angleOffsets[0];
		var bentVertex0 = {position: p0, angleOffset: angleOffset, angle: angle0 + angleOffset, straightIndex: vertexIndex0, bentId: bentId};
		bentVertices.push(bentVertex0);
		var position = bentVertex0.position;
		var angleOffset = bentVertex0.angleOffset;

		var bentIndices = [bentIndex0];
		var lineToBentVertices = {line: line, bentIndices: bentIndices, isForward: endpoint0.getIsAtStart()};
		linesToBentVertices.push(lineToBentVertices);
		for (var s = 0; s < numSegments; s++) {
			var angle = ms.util.fixAngle(angle0 + angleOffset);
			var delta = ms.vec2.unitVec(angle).scale(segmentLength);
			position = position.copy().add(delta);
			bentIndices.push(bentVertices.length);

			var bentId;
			var straightIndex;
			if (s == numSegments - 1) {
				straightIndex = vertexIndex1;
				if (bentIndex1 == -1) {
					bentId = bentVertices.length;
					straightToBentMap[vertexIndex1] = bentVertices.length;
					if (freeCoreVertices.includes(endpoint1.getVertex())) {
						endpoint1.getVertex().setPosition(position);
					}
				} else {
					bentId = straightToBentMap[vertexIndex1];
				}
			} else {
				straightIndex = -1;
				bentId = bentVertices.length;
			}
			
			bentVertices.push({position: position, angleOffset: angleOffset, angle: angle, straightIndex: straightIndex, bentId: bentId});
			angleOffset = angleOffset + angleOffsets[s + 1];
		}
	}
	// Create maps between the bent vertices and the solver vertices. Handle fixed vertices
	// differently. These map to the original straight vertices.
	var bentToSolver = [];
	var solverToBent = [];
	var solverVertexCount = 0;
	for (var i = 0; i < bentVertices.length; i++) {
		var v = bentVertices[i];
		if (v.bentId == i) {
			var straightIndex = v.straightIndex;
			var isFree = (straightIndex < 0) || this.freeVertices.includes(vertices[straightIndex]);
			var vertex = (straightIndex >= 0) && vertices[straightIndex].getCore();
			if (vertex) {
				var isBoundary = vertex.getEndpoints().some(function(endpoint) {
					return endpoint.getEdgeType().isBoundary();
				});
				isFree = isFree && !isBoundary;
			}			
			if (isFree) {
				bentToSolver[i] = {free: solverVertexCount, fixed: -1};
				solverToBent[solverVertexCount] = i;
				solverVertexCount++;
			} else {
				bentToSolver[i] = {free: -1, fixed: straightIndex};
			}
		}
	}
	var A = new ms.matrix();
	var W = new ms.matrix();
	var x = new ms.matrix();
	var row = 0;
	for (var i = 0; i < linesToBentVertices.length; i++) {
		var bentIndices = linesToBentVertices[i].bentIndices;
		for (var j = 0; j < bentIndices.length - 1; j++) {
			var originalVertex0 = bentVertices[bentIndices[j    ]];
			var originalVertex1 = bentVertices[bentIndices[j + 1]];
			var position0 = originalVertex0.position;
			var position1 = originalVertex1.position;
			var bentId0 = bentVertices[originalVertex0.bentId].bentId;
			var bentId1 = bentVertices[originalVertex1.bentId].bentId;
			var solver0 = bentToSolver[bentId0];
			var solver1 = bentToSolver[bentId1];
			
			if (solver0.free == -1 && solver1.free == -1) {
				continue;
			}
			var change = position1.copy().minus(position0);
			if (solver0.free >= 0) {
				A.addValue(row,     2 * solver0.free, -1);
				A.addValue(row + 1, 2 * solver0.free + 1, -1);
			} else {
				change.add(vertices[solver0.fixed].getCore().getPosition());
			}
			if (solver1.free >= 0) {
				A.addValue(row,     2 * solver1.free, 1);
				A.addValue(row + 1, 2 * solver1.free + 1, 1);
			} else {
				change.minus(vertices[solver1.fixed].getCore().getPosition());
			}
			W.addValue(row, row, 1);
			W.addValue(row + 1, row + 1, 1);
			x.addValue(row, 0, change.x);
			x.addValue(row + 1, 0, change.y);
			row += 2;
		}
	}
	// All the vertices except the outer vertex is free.
	if (vertices.length == this.freeVertices.length + 1) {
		if (bentVertices[0].bentId != 0) {
			ms.alert('First Id is not zero.');
		}
		A.addValue(row, 0, 1);
		A.addValue(row + 1, 1, 1);
		W.addValue(row, row, 1);
		W.addValue(row + 1, row + 1, 1);
		var p = bentVertices[0].position;
		x.addValue(row, 0, p.x);
		x.addValue(row + 1, 0, p.y);
	}
	if (row == 0) {
		ms.alert('Rows empty in bendLines');
		return false;
	}
	
	var c = ms.leastSquares.leastSquares(A, W, x);
	c = c.asFull();
	for (var i = 0; i < solverToBent.length; i++) {
		var v = bentVertices[solverToBent[i]];
		v.position = new ms.vec2(c[2 * i][0], c[2 * i + 1][0]);
		if (!this.mutationArea.isInside(v.position)) {
			return false;
		}
	}
	for (var i = 0; i < solverToBent.length; i++) {
		var bentVertex = bentVertices[solverToBent[i]];
		if (bentVertex.straightIndex >= 0) {
			var straightVertex = vertices[bentVertex.straightIndex];
			if (this.freeVertices.includes(straightVertex)) {
				straightVertex.getCore().setPosition(bentVertex.position.copy());
			}
		}
	}
	
	// Copy the fixed vertices.
	for (var i = 0; i < bentVertices.length; i++) {
		if (bentToSolver[i] && bentToSolver[i].fixed >= 0) {
			bentVertices[i].position = vertices[bentToSolver[i].fixed].getCore().getPosition().copy();
		}
	}
	
	var carefulBending = ms.globalSettings.get('Careful Bending');
	for (var i = 0; i < linesToBentVertices.length; i++) {
		var map = linesToBentVertices[i];
		var isForward = map.isForward;
		var segment = map.line.getSegment();
		var state = ms.pickOne(segment.getStates());
		state.getNode().disconnect(segment);
		state.destroy();
		
		var bentIndices = map.bentIndices;
		var stateTs = [0];
		var cumulativeLength = 0;
		for (var j = 0; j < bentIndices.length - 1; j++) {
			var vertex0a = bentVertices[bentIndices[j]];
			var vertex1a = bentVertices[bentIndices[j + 1]];
			var vertex0p = bentVertices[vertex0a.bentId];
			var vertex1p = bentVertices[vertex1a.bentId];
			var stateLength = vertex0p.position.distance(vertex1p.position);
			cumulativeLength += stateLength;
			stateTs.push(cumulativeLength);
		}
		
		var states = [];
		var t = 0;

		var brush = map.line.getEdgeType().getBrush();
		var lengthInTiles = brush ? cumulativeLength / brush.get('Tile Length') : 1;
		var numTiles = Math.max(1, Math.round(lengthInTiles));
		var tScale = numTiles / cumulativeLength;
		
		for (var j = 0; j < bentIndices.length - 1; j++) {
			var vertex0a = bentVertices[bentIndices[j]];
			var vertex1a = bentVertices[bentIndices[j + 1]];
			var vertex0p = bentVertices[vertex0a.bentId];
			var vertex1p = bentVertices[vertex1a.bentId];
			var t0 = tScale * stateTs[j];
			var t1 = tScale * stateTs[j + 1];
			var angledEdges = [
				new ms.angledEdge(vertex0p.position, vertex0a.angle, t0),
				new ms.angledEdge(vertex1p.position, ms.util.fixAngle(vertex1a.angle + Math.PI), t1)
			];
			if (!isForward) {
				angledEdges.reverse();
			}
			states.push(new ms.lineState(this.stats, new ms.lineStateCoordinates(angledEdges, 1)));
		}		
		if (!isForward) {
			states.reverse();
		}
		segment.addStates(states);
		for (var j = 0; j < states.length; j++) {
			if (carefulBending) {
				var intersection = states[j].addToModelWithIntersections(-Infinity);
				if (intersection) {
					return false;
				}
			} else {
				states[j].addToModel();
			}
		}
	}
	return true;
}

ms.transistor.prototype.placeVertexPositions = function(positions) {
	var freeVertices = this.freeVertices;
	for (var i = 0; i < freeVertices.length; i++) {
		var position = this.dims == 2 ?
			new ms.vec2(positions[this.dims * i][0], positions[this.dims * i + 1][0]) :
			new ms.vec3(positions[this.dims * i][0], positions[this.dims * i + 1][0], positions[this.dims * i + 2][0]);
		freeVertices[i].getCore().setPosition(position);
		if (!this.model.inBounds(position.x, position.y, position.z)) {
			return false;
		}
	}	
	
	// Faces to update the face connection.
	var facesToUpdate = new Set();
	
	// Skip 3D intersections for now.
	if (this.dims == 2) {
		for (var i = 0; i < this.propagationOrder.length; i++) {
			var endpoint = this.propagationOrder[i];
			var line = endpoint.getEdge().getCore();
			line.moveToEndpoints();
			var lineState = line.getSegment().getStates()[0];
			var intersection = lineState.addToModelWithIntersections(-Infinity);
			if (intersection) {
				if (!intersection.state) {
					return false;
				}
				var intersectedLine = intersection.state.getLine();
				var matched = this.edgeBlockers.some(function(blocker) {
					return blocker.matches(endpoint, intersectedLine);
				});
				if (matched) {
					ms.alert('Violating an existing edge blocker.');
				}
				// TODO: Fix the behavior for moving lines.
				var blockerMoving = this.freeEdges.some(function(freeEdge) { return freeEdge.getCore() == intersectedLine; });
				if (blockerMoving) {
					return false;
				}
				var blocker = this.createEdgeBlocker(endpoint, intersectedLine);
				blocker && this.edgeBlockers.push(blocker);
				// Reset the line states, remove them from the model.
				for (var j = 0; j <= i; j++) {				
					var endpoint = this.propagationOrder[j];
					var line = endpoint.getEdge().getCore();
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
	
	var getCore = function(v) { return v.getCore(); };
	var coreVertices1 = this.openPaths.map(function(path) { return path.endpoints[0].getVertex(); });
	var coreVertices2 = freeVertices.map(getCore);
	var coreVertices3 = this.graph.getVertices().filter(function(v) { return v.isBoundary(); }).map(getCore);
	var coreVertices = coreVertices1.concat(coreVertices2).concat(coreVertices3);
	coreVertices.forEach(function(v) {
		v.getEndpoints().forEach(function(endpoint) {
			endpoint.maybeMergeNextFace();
		});
	});

	if (this.dims == 2) {
		// Find all the faces that were involved.
		var addFaces = function(endpoint) {
			facesToUpdate.add(endpoint.getFace());
			endpoint.getConnections().forEach(function(connection) {
				facesToUpdate.add(connection.getFace());
			});
		}
		this.freeEdges.forEach(function(edge) {
			var endpoints = edge.getCore().getEndpoints();
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
		
		if (ms.globalSettings.get('Bend Lines') && ms.guideMutator.taskCount > ms.globalSettings.get('Bend Time Start')) {
			var success = this.bendLines();
			if (!success) {
				return false;
			}
		} else {
			this.freeEdges.forEach(function(graphEdge) {
				var edge = graphEdge.getCore();
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
		}
	} else {
		var facesToIntersect = [];
		coreVertices.forEach(function(v) {
			v.getEndpoints().forEach(function(endpoint) {
				var face = endpoint.getFace();
				
				if ((face.getPolygons().length == 0) && !facesToIntersect.includes(face)) {
					facesToIntersect.push(face);
				}
			});
		});
		var tree = this.model.getBspTree();
		for (var i = 0; i < facesToIntersect.length; i++) {
			var face = facesToIntersect[i]; 
			var plane = ms.bspPlane.create(face);
			var polygon = ms.bspPolygon.create(face);
			if (polygon.selfIntersects()) {
				for (j = 0; j <= i; j++) {
					facesToIntersect[j].getPolygons()[0].getNode().destroy();
				}
				return false;
			}
			var success = tree.add(plane, polygon);
			if (!success) {
				return false;
			}
		}
	}
	return true;
};

ms.transistor.prototype.print = function() {
	// Not sure why we need to use vertices here.
	this.graph.vertices[0].print();
};

ms.transistor.prototype.reject = function() {
	if (!this.graph) {
		return;
	}
	// Set the cost really high.
	this.stats.costChange['reject'] = 1e6;
};

ms.transistor.prototype.createEdgeBlocker = function(endpoint, intersectedLine) {
	// There are no degress of freedom here.
	if (this.changeBasis.size()[0] == 0 || this.changeBasis.size()[1] == 0) {
		this.effort = Infinity;
		return null;
	}
	
	var pIndex = 2 * this.freeVertices.indexOf(endpoint.twin().getVertex());
	if (pIndex < 0) {
		endpoint = endpoint.twin();
		pIndex = 2 * this.freeVertices.indexOf(endpoint.twin().getVertex());
		if (pIndex < 0) {
			this.effort = Infinity;
			return null;
		}
	}
	var angleU = endpoint.getEdge().getCore().getAngle() + Math.PI / 2;
	var angleV = intersectedLine.getAngle() + Math.PI / 2;
	var u = ms.vec2.unitVec(angleU);
	var v = ms.vec2.unitVec(angleV);
	if (v.dot(ms.vec2.unitVec(endpoint.getCore().getAngle()))) {
		v.scale(-1);
	}
	var u0 = u.dot(intersectedLine.getEndpoints()[0].getVertex().getPosition());
	var u1 = u.dot(intersectedLine.getEndpoints()[1].getVertex().getPosition());
	var v0 = v.dot(intersectedLine.getEndpoints()[0].getVertex().getPosition());
	if (u0 > u1) {
		var temp = u0
		u0 = u1;
		u1 = temp;
	}
	
	var initialValues = this.initialPosition.valueOf();
	var Q = mathG.matrix([[u.x, u.y], [v.x, v.y]]);
	var P = this.changeBasis.subset(mathG.index(ms.util.sequence(pIndex, pIndex + 1), ms.util.sequence(0, this.changeBasis.size()[1] - 1)));
	var R = mathG.multiply(Q, P);
	var P0 = mathG.matrix([[initialValues[pIndex][0]], [initialValues[pIndex + 1][0]]]);
	var Q0 = mathG.multiply(Q, P0);
	u0 -= Q0.valueOf()[0][0];
	u1 -= Q0.valueOf()[0][0];
	v0 -= Q0.valueOf()[1][0];
	return new ms.transistorEdgeBlocker(endpoint, intersectedLine, R, u0, u1, v0);
};

ms.transistorEdgeBlocker = function(endpoint, intersectedLine, R, u0, u1, v0) {
	this.endpoint = endpoint;
	this.intersectedLine = intersectedLine;
	this.R = R;
	this.u0 = u0;
	this.u1 = u1;
	this.v0 = v0;
	this.id = ms.transistorEdgeBlocker.count++;
};

ms.transistorEdgeBlocker.count = 0;

ms.transistorEdgeBlocker.prototype.matches = function(endpoint, intersection) {
	return this.endpoint == endpoint && this.intersection == intersection;
};

ms.transistorEdgeBlocker.prototype.isBlocking = function(samples) {
	var result = mathG.multiply(this.R, samples).valueOf();
	var u = result[0][0];
	var v = result[1][0];
	return this.u0 < u && u < this.u1 && v > this.v0;
};
