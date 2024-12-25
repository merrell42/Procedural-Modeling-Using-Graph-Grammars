// A generic graph.
ms.graph = function() {
	this.vertices = [];
	this.edges = [];
	this.links = {};
	this.signature = '';

	// The outer vertex is really a fake vertex for all the loose edges
	// that aren't actually connected to a vertex.
	this.outerVertex = new ms.graphVertex(new ms.outerVertex());
	this.addVertex(this.outerVertex);

	this.id = ms.graph.count++;
};

ms.graph.count = 0;

ms.graph.DEFAULT_LENGTH = 50;
ms.graph.LEAF_LENGTH = 10;
ms.graph.COMPONENT_DISTANCE = 100;
ms.graph.HIGHLIGHTED_SIZE = 100;
ms.graph.FACE_EDGE_WIDTH = 6;
ms.graph.FACE_WIDTH_3D = 5;

ms.graph.prototype.export = function(types) {
	var endpoints = [];
	this.vertices.forEach((v) => {
		ms.union(endpoints, v.getEndpoints());
	});
	var outerEndpoints = this.getOuterEndpoints().map((endpoint) => {
		return endpoints.indexOf(endpoint);
	});
	return {
		vertices: this.vertices.map((v) => v.export(types)),
		edges: this.edges.map((e) => e.export(types)),
		endpoints: endpoints.map((e) => e.export(types)),
		outerEndpoints,
	};
};

ms.graph.import = function(json, types) {
	var graph = new ms.graph();
	graph.vertices = json.vertices.map((v) => {
		var vertex = ms.graphVertex.import(v, types);
		vertex.setGraph(graph);
		return vertex;
	});
	graph.outerVertex = graph.vertices[0];
	graph.edges = json.edges.map((e) => {
		var edge = ms.graphEdge.import(e, types);
		edge.setGraph(graph);
		return edge;
	});
	var endpoints = json.endpoints.map((endpoint) => ms.graphEndpoint.import(endpoint, graph));
	// The outer endpoints must be in a particular order for everything to work.
	graph.outerVertex.endpoints = json.outerEndpoints.map((endpoint) => endpoints[endpoint]);
	return graph;
};

ms.graph.prototype.getOuterVertex = function() {
	return this.outerVertex;
};

ms.graph.prototype.getOuterEndpoints = function() {
	return this.outerVertex.getEndpoints();
};

ms.graph.prototype.getVertices = function() {
	return this.vertices;
};

ms.graph.prototype.getEdges = function() {
	return this.edges;
};

ms.graph.prototype.getFaceEndpoints = function() {
	return this.faceEndpoints;
};

ms.graph.prototype.isEmpty = function() {
	return (this.edges.length + this.vertices.length) == 1;
};

ms.graph.prototype.getLink = function(key) {
	return this.links[key];
};

ms.graph.prototype.setLink = function(key, value) {
	this.links[key] = value;
};

ms.graph.prototype.possiblyEquivalent = function(graphB) {
	if (this.vertices.length != graphB.getVertices().length) {
		return false;
	}
	if (this.edges.length != graphB.getEdges().length) {
		return false;
	}
	if (this.getOuterEndpoints().length != graphB.getOuterEndpoints().length) {
		return false;
	}
	return true;
};

ms.graph.prototype.addVertex = function(vertex) {
	this.signature = '';
	if (!this.vertices.includes(vertex)) {
		this.vertices.push(vertex);
		vertex.setGraph(this);
	}
};

ms.graph.prototype.addEdge = function(edge, vertex0, vertex1) {
	this.signature = '';
	this.addVertex(vertex0);
	this.addVertex(vertex1);
	this.edges.push(edge);
	edge.setGraph(this);
	
	var endpoint0 = new ms.graphEndpoint(vertex0, edge);
	var endpoint1 = new ms.graphEndpoint(vertex1, edge);
	edge.setEndpoint(endpoint0, 0);
	edge.setEndpoint(endpoint1, 1);
	vertex0.addEndpoint(endpoint0);
	vertex1.addEndpoint(endpoint1);
};

ms.graph.prototype.removeEdge = function(edge) {
	this.signature = '';
	edge.setGraph(null);
	ms.remove(edge, this.edges);
	edge.getVertex(0).removeEdge(edge);
	edge.getVertex(1).removeEdge(edge);
	edge.setEndpoint(null, 0);
	edge.setEndpoint(null, 1);
};

ms.graph.prototype.removeVertex = function(vertex) {
	if (vertex.getEndpoints().length > 0) {
		ms.alert('Removed vertex should have no endpoints.');
	}
	ms.remove(vertex, this.vertices);
};

// Create a graph with a single edge in it.
ms.graph.createSingleEdgeGraph = function(edgeType) {
	var graph = new ms.graph();
	graph.addEdge(new ms.graphEdge(edgeType), graph.getOuterVertex(), graph.getOuterVertex());
	return graph;
};

// Create a graph with a single vertex in it.
ms.graph.createSingleVertexGraph = function(vertexType) {
	var graph = new ms.graph();
	var graphVertex = new ms.graphVertex(vertexType);
	var connections = vertexType.getConnections();
	
	for (var i = 0; i < connections.length; i++) {
		var connection = connections[i];
		var edge = new ms.graphEdge(connection.edge);
		if (connection.isAtStart) {
			graph.addEdge(edge, graphVertex, graph.getOuterVertex());
		} else {
			graph.addEdge(edge, graph.getOuterVertex(), graphVertex);
		}
	}
	
	// console.log(graphVertex.endpoints.map((e) => e.edge.core.id));
	// console.log(connections.map((e) => e.edge.id));
	
	var copy = graph.copy();
	return graph;
};

ms.graph.prototype.createVertexFromOption = function(endpoint, option) {
	var newVertex = new ms.graphVertex(option.vertex);
	var attachmentVertex = endpoint.twin().getVertex();
	this.removeEdge(endpoint.edge);
	var connections = option.vertex.getConnections();
	for (var i = 0; i < connections.length; i++) {
		var connection = connections[i];
		var edge = new ms.graphEdge(connection.edge);
		var otherVertex = (i == option.index) ? attachmentVertex : this.getOuterVertex();
		if (connection.isAtStart) {
			this.addEdge(edge, newVertex, otherVertex);
		} else {
			this.addEdge(edge, otherVertex, newVertex);
		}
	}
};

ms.graph.prototype.merge = function(graphB) {
	this.signature = '';
	var getOuterEdgeIndices = function(graph) {
		return graph.getOuterEndpoints().map(function(endpoint) {
			return 2 * graph.getEdges().indexOf(endpoint.getEdge()) + endpoint.getEdgeIndex();
		});
	};
	// Save the old index positions.
	var oldIndices = getOuterEdgeIndices(this);
	var oldEdgeCount = this.getEdges().length;

	var self = this;
	var bVertices = graphB.getVertices();
	var bOuterVertex = graphB.getOuterVertex();
	var newVertices = bVertices.map(function(vertex) {
		if (vertex == bOuterVertex) {
			return self.outerVertex;
		} else {
			var newVertex = new ms.graphVertex(vertex.getCore());
			self.addVertex(newVertex);
			return newVertex;
		}
	});
	var newEdges = graphB.getEdges().map(function(edge) {
		return new ms.graphEdge(edge.getCore());
	});
	graphB.getEdges().forEach(function(edgeB, index) {
		var newVertex0 = newVertices[bVertices.indexOf(edgeB.getVertex(0))];
		var newVertex1 = newVertices[bVertices.indexOf(edgeB.getVertex(1))];
		self.addEdge(newEdges[index], newVertex0, newVertex1);
	});

	// Reorder the outer endpoints so that the new endpoints are at the end.
	var bIndices = getOuterEdgeIndices(graphB);
	bIndices = bIndices.map(function(bIndex) {
		return bIndex + 2 * oldEdgeCount;
	});
	oldIndices = oldIndices.concat(bIndices);
	var newIndices = getOuterEdgeIndices(this);
	var newOrdering = newIndices.map(function(bIndex) { return oldIndices.indexOf(bIndex); });
	this.getOuterVertex().reorderEndpoints(newOrdering);
};

ms.graph.prototype.copy = function() {
	var newGraph = new ms.graph();
	newGraph.merge(this);
	var keys = Object.keys(this.links);
	for (var i = 0; i < keys.length; i++) {
		var key = keys[i];
		newGraph.setLink(key, this.links[key]);
	}
	return newGraph;
};

// Returns a copy of the graph along with the replacement endpoints.
ms.graph.prototype.copyEndpoints = function(oldEndpoints) {	
	var indices = oldEndpoints.map(function(e) {
		return e.outerIndex();
	});
	var newGraph = this.copy();
	var newEndpoints = newGraph.getOuterEndpoints();
	var replacementEndpoints = indices.map(function(index) {
		return newEndpoints[index];
	});
	return replacementEndpoints;
};

ms.graph.faceEndpoints = function(endpoint) {
	var endpoints = [endpoint];
	while (!endpoint.twin().getVertex().isOuter()) {
		endpoint = endpoint.next();
		endpoints.push(endpoint);
	}
	return endpoints;
};

// Get the face endpoints when the face is a loop.
ms.graph.faceEndpointsLoop = function(endpoint0) {
	var endpoint = endpoint0;
	var endpoints = [];
	endpoint = endpoint.next();
	endpoints.push(endpoint);
	while (endpoint != endpoint0) {
		endpoint = endpoint.next();
		endpoints.push(endpoint);
	}
	return endpoints;
};

// Get the face endpoints when the face is a loop.
ms.graph.faceEndpointsLoop3D = function(endpoint0, fType) {
	var endpoint = endpoint0;
	var endpoints = [];
	endpoint = endpoint.nextOnFace(fType);
	endpoints.push(endpoint);
	while (endpoint != endpoint0) {
		endpoint = endpoint.nextOnFace(fType);
		endpoints.push(endpoint);
	}
	return endpoints;
};

// TODO: Not sure what the winding angle does when we reverse. This is just to find the exit.
ms.graph.traceToExit = function(endpoint, opt_reverse) {
	var winding = 0;
	var prevDifference;
	if (opt_reverse) {
		endpoint = endpoint.twin();
	}
	var prevAngle = endpoint.getAngle();
	while (!(opt_reverse ? endpoint : endpoint.twin()).getVertex().isOuter()) {
		if (opt_reverse) {
			endpoint = endpoint.prev();
		} else {
			endpoint = endpoint.next();
		}
		nextAngle = endpoint.getAngle();
		var difference = ms.util.angleDifference(prevAngle, nextAngle);
		if (difference < -Math.PI + 1e-5) {
			difference += 2 * Math.PI;
		}
		winding += difference;
		prevAngle = nextAngle;
	}
	return { winding: winding, exit: endpoint };
};

// I would prefer to just use the turns, but at the moment this is necessary for
// determing if we turn more than 360 degrees.
// Similar to turnToExit.
ms.graph.traceToExit3D = function(endpoint, fType) {
	var winding = 0;
	var updateWinding = function(angle, nextAngle) {
		var difference = ms.util.angleDifference(angle, nextAngle);
		if (difference < -Math.PI + 1e-5) {
			difference += 2 * Math.PI;
		}
		winding += difference;
	};

	var next = endpoint.nextOnFace(fType);
	var angle = endpoint.getFaceAngle(fType);
	var nextAngle = next ? next.getFaceAngle(fType) : angle;
	updateWinding(angle, nextAngle);
	while (next) {
		endpoint = next;
		next = endpoint.nextOnFace(fType);
		if (next) {
			angle = nextAngle;
			nextAngle = next.getFaceAngle(fType);
			updateWinding(angle, nextAngle);
		}
	}
	return { winding: winding, exit: endpoint };
};

// Same as traceToExit, but returns the number of turns.
// There may be a way to optimize this so we don't call it so much.
ms.graph.turnToExit = function(endpoint, fType) {
	var next = endpoint.nextOnFace(fType);
	var angle = endpoint.getFaceAngle(fType);
	var nextAngle = next ? next.getFaceAngle(fType) : angle;
	var turns = ms.util.angleTurn(angle, nextAngle);
	while (next) {
		endpoint = next;
		next = endpoint.nextOnFace(fType);
		if (next) {
			angle = nextAngle;
			nextAngle = next.getFaceAngle(fType);
			turns += ms.util.angleTurn(angle, nextAngle);
		}
	}
	return { turns, exit: endpoint };
};

ms.graph.windingNumber = function(endpoint) {
	return ms.graph.traceToExit(endpoint).winding / (2 * Math.PI);
};


ms.graph.prototype.findOverlyTurned = function(endpoint) {
	var overlyTurned = [];
	if (this.getLink('is3D')) {
		this.getOuterEndpoints().forEach(function(endpoint) {
			var isTurned = false;
			var faceData = endpoint.getEdge().getCore().getFaceData();
			faceData.forEach(function(faceDatum) {
				if (isTurned) {
					return;
				}
				var fType = faceDatum.type
				var trace = ms.graph.traceToExit3D(endpoint, fType);
				if (Math.abs(trace.winding) > 2 * Math.PI + 1e-5) {
					isTurned = true;
					// I don't know if this is the right endpoint. Not tested.
					overlyTurned.push(endpoint);
				}
			});
		});
	} else {
		var allWindings = this.endpointWindings();
		allWindings.forEach(function(windings) {	
			windings.push({winding: 2 * Math.PI});
			for (var i = 0; i < windings.length - 1; i++) {
				var turning = windings[i + 1].winding - windings[i].winding - Math.PI;
				if (turning < -2 * Math.PI - 1e-5) {
					overlyTurned.push(windings[i].endpoint);
				}
			}
		});
	}
	return overlyTurned;
};

// TODO: Delete.
ms.graph.prototype.endpointWindings = function() {
	var outerEndpoints = this.getOuterEndpoints();
	var traced = new Array(outerEndpoints.length).fill(false);
	var untraced = traced.findIndex(function(t) { return !t; });
	var groups = [];
	while (untraced >= 0) {
		var endpoint0 = outerEndpoints[untraced];
		var endpoint = endpoint0;
		var winding = 0;
		var windings = [];
		var next = function() {
			var index = endpoint.outerIndex();
			traced[index] = true;
			windings.push({endpoint: endpoint, winding: winding});
			var result = ms.graph.traceToExit(endpoint);
			endpoint = result.exit.twin();
			winding += result.winding + Math.PI;
		}
		next();
		while (endpoint != endpoint0) {
			next();
		}
		untraced = traced.findIndex(function(t) { return !t; });
		groups.push(windings);
	}
	return groups;
};

ms.graph.prototype.singleFragmentCount = function() {
	var hasSingleFragment = this.getEdges().some(function(edge) {
		return edge.getCore().singleFragment();
	});
	return hasSingleFragment ? 1 : 0;
}

// Measure the winding in the reverse direction.
ms.graph.reverseWinding = function(endpointA, endpointB) {
	var endpoint = endpointA;
	var winding = 0;
	while (endpoint != endpointB) {
		var result = ms.graph.traceToExit(endpoint);
		endpoint = result.exit.twin();
		winding += result.winding + Math.PI;
	}
	return winding;
};

ms.graph.prototype.findInnerLoops = function() {
	var edges = this.getEdges().slice();
	edges = edges.filter(function(e) {
		return !e.getVertex(0).isOuter() && !e.getVertex(1).isOuter();
	});
	var endpoints = [];
	for (var i = 0; i < edges.length; i++) {
		endpoints.push(edges[i].getEndpoint(0));
		endpoints.push(edges[i].getEndpoint(1));
	}

	var loops = [];
	while (endpoints.length > 0) {
		var initialEndpoint = endpoints.pop();
		var endpoint = initialEndpoint;

		var success;
		var loop = [];
		var prevAngle = endpoint.getAngle();
		var prevDifference = -1;
		var winding = 0;
		endpoint = endpoint.next();
		while (true) {
			loop.push(endpoint);
			nextAngle = endpoint.getAngle();
			if (endpoint.twin().getVertex().isOuter()) {
				success = false;
				break;
			}
			var difference = ms.util.angleDifference(prevAngle, nextAngle);
			if (difference < -Math.PI + 1e-5) {
				difference += 2 * Math.PI;
			}
			winding += difference;

			if (endpoint == initialEndpoint) {
				success = true;
				break;
			}
			prevAngle = nextAngle;
			endpoint = endpoint.next();
		}
		if (success) {
			winding = winding / (2 * Math.PI);
			if (Math.abs(Math.abs(winding) - 1) > 0.001) {
				ms.alert('winding number is wrong.');
			}
			if (winding < 0) {
				loops.push(loop);
			}
		}
		if (loop) {
			for (var i = 0; i < loop.length; i++) {
				ms.maybeRemove(loop[i], endpoints);
			}
		}
	}
	return loops;
};

// TODO: Combine with findInnerLoops.
ms.graph.isInsideLoop = function(initialEndpoint) {
	var endpoint = initialEndpoint;

	var success;
	var loop = [];
	var prevAngle = endpoint.getAngle();
	var prevDifference = -1;
	var winding = 0;
	endpoint = endpoint.next();
	while (true) {
		loop.push(endpoint);
		nextAngle = endpoint.getAngle();
		if (endpoint.twin().getVertex().isOuter()) {
			success = false;
			break;
		}
		var difference = ms.util.angleDifference(prevAngle, nextAngle);
		if (difference < -Math.PI + 1e-5) {
			difference += 2 * Math.PI;
		}
		winding += difference;

		if (endpoint == initialEndpoint) {
			success = true;
			break;
		}
		prevAngle = nextAngle;
		endpoint = endpoint.next();
	}
	if (success) {
		winding = winding / (2 * Math.PI);
		if (Math.abs(Math.abs(winding) - 1) > 0.001) {
			ms.alert('winding number is wrong.');
		}
		if (winding < 0) {
			// loops.push(loop);
			return true;
		}
	}
	return false;
};

// Modify the graph so that endpointA points where endpointB used to
// point to. And vice versa.
ms.graph.prototype.splice = function(endpointA, endpointB) {
	var oldEdgeA = endpointA.getEdge();
	var oldEdgeB = endpointB.getEdge();
	var vA0 = endpointA.getVertex();
	var vA1 = endpointA.twin().getVertex();
	var vB0 = endpointB.getVertex();
	var vB1 = endpointB.twin().getVertex();
	
	var newEdgeA = new ms.graphEdge(oldEdgeA.getCore());
	var newEdgeB = new ms.graphEdge(oldEdgeB.getCore());
	this.removeEdge(oldEdgeA);
	this.removeEdge(oldEdgeB);
	this.addEdge(newEdgeA, vA0, vB1);
	this.addEdge(newEdgeB, vB0, vA1);
	return [newEdgeA.getEndpoint(0), newEdgeA.getEndpoint(1)];
};

ms.graph.prototype.highlight = function(view, opt_options) {
	var options = opt_options || {};
	var size = ms.graph.HIGHLIGHTED_SIZE;
	var offset = options.offset || new ms.vec2(20, view.canvas.height - size - 20);
	options.rect = [offset.x, offset.x + size, offset.y, offset.y + size, 0, 100];
	this.draw(view, options);
};

ms.graph.prototype.requiresShapeView = function () {
	return true;
};

// Replace each edge to the outer vertex with an edge to a new vertex.
ms.graph.prototype.replaceOuterEdges = function(createVertexCore) {
	// The edges in their original order.
	var orderedEdges = this.edges.slice();
	var outerVertex = this.getOuterVertex();
	while (outerVertex.getEndpoints().length > 0) {
		var endpoint = outerVertex.getEndpoints()[0];
		var newVertex = new ms.graphVertex(createVertexCore());
		var oldVertex = endpoint.twin().getVertex();
		var originalIndex = orderedEdges.indexOf(endpoint.edge);
		this.removeEdge(endpoint.edge);
		var newEdge = new ms.graphEdge(endpoint.edge.getCore());
		if (endpoint.getEdgeIndex() == 0) {
			this.addEdge(newEdge, newVertex, oldVertex);
		} else {
			this.addEdge(newEdge, oldVertex, newVertex);			
		}
		orderedEdges[originalIndex] = newEdge;
	}
	this.edges = orderedEdges;
};

// Find groups of connected components.
ms.graph.prototype.connectedComponents = function() {
	var components = [];
	var vertices = this.vertices.filter(function(vertex) { return !vertex.isOuter(); });
	while (vertices.length > 0) {	
		var newVertex = vertices.shift();
		var groupVertices = [newVertex];
		var groupEdges = [];
		var newVertices = [newVertex];
		while (newVertices.length > 0) {
			var v = newVertices.shift();
			v.getEndpoints().forEach(function(endpoint) {
				var twin = endpoint.twin();
				var twinVertex = twin.getVertex();
				var twinEdge = twin.getEdge();
				if (!groupVertices.includes(twinVertex)) {
					ms.remove(twinVertex, vertices);
					groupVertices.push(twinVertex);
					newVertices.push(twinVertex);
				}
				if (!groupEdges.includes(twinEdge)) {
					groupEdges.push(twinEdge);
				}
			});
		}
		components.push({vertices: groupVertices, edges: groupEdges});
	}
	return components;
};

// Put disconnected pieces into separate graphs.
ms.graph.prototype.disconnect = function() {
	var result = [];
	var components = this.connectedComponents();
	for (var i = 0; i < components.length; i++) {	
		var newGraph = new ms.graph();
		newGraph.vertices = components[i].vertices;
		newGraph.edges = components[i].edges;
		result.push(newGraph.copy());
	}
	return result;
};

ms.graph.prototype.equals = function(graphB) {
	if (this.vertices.length != graphB.vertices.length) {
		return false;
	}
	if (this.edges.length != graphB.edges.length) {
		return false;
	}
	for (var i = 0; i < this.vertices.length; i++) {
		if (!this.vertices[i].equals(graphB.vertices[i])) {
			return false;
		}
	}
	for (var i = 0; i < this.edges.length; i++) {
		if (!this.edges[i].equals(graphB.edges[i])) {
			return false;
		}
	}
	return true;
};

ms.graph.prototype.print = function() {
	ms.highlight(this);
};

ms.graph.drawEdge = function(edge, context, view, color, scale, center, rectCenter, offsetScale) {
	var position0 = edge.getVertex(0).getCore().position;
	var position1 = edge.getVertex(1).getCore().position;
	// Note that the y-axis is reversed.		
	var startPos = new ms.vec2(scale * (position0.x - center.x) + rectCenter.x, scale * (center.y - position0.y) + rectCenter.y);
	var endPos =   new ms.vec2(scale * (position1.x - center.x) + rectCenter.x, scale * (center.y - position1.y) + rectCenter.y);
	var dir = endPos.copy().minus(startPos);
	dir.normalize();
	var offset = new ms.vec2(-dir.y, dir.x);
	offset.scale(offsetScale * ms.graph.FACE_EDGE_WIDTH / 3);

	context.beginPath();
	var lineOptions = { hasArrows: [false, false], offset: offset };
	view.drawLine(color, startPos, endPos, new ms.vec2(0, 0), lineOptions);
	context.stroke();
};

ms.graph.prototype.draw = function(view, options) {
	var color = options.color || '#1d3c6b';
	var rect = options.rect;
	var rectCenter = new ms.vec2((rect[0] + rect[1]) / 2, (rect[2] + rect[3]) / 2);
	var rectExtents = new ms.vec2((rect[1] - rect[0]), (rect[3] - rect[2]));
	if (this.isEmpty()) {
		view.context.fillStyle = '#fff';
		view.context.strokeStyle = '#fff';
		var r = rectExtents.x / 4;
		view.drawCircleOld(rectCenter, ms.vec2.ORIGIN, r);
		var startPos = rectCenter.copy();
		var endPos = rectCenter.copy();
		startPos.move(r, -r);
		endPos.move(-r, r);
		view.drawLine('fff', startPos, endPos, ms.vec2.ORIGIN, {});
		return;
	}
	
	var lineWidth = options.lineWidth || 2;
	var positionGraph = this.copy();
	positionGraph.getVertices().forEach(function(vertex) {
		vertex.setCore({position: null, isOuter: false, compare: function() {return true;}});
	});

	// Replace each edge to the outer vertex with an edge to a new vertex.
	var createVertexCore = function() {
		return {position: null, isOuter: true, compare: function() {return true;}};
	};
	positionGraph.replaceOuterEdges(createVertexCore);
	var positionEdges = positionGraph.getEdges().slice();

	// Return if the graph is empty.
	if (positionGraph.isEmpty()) {
		return;
	}
	// Don't use the outer vertex. It's not connected to anything.
	var verticesToAdd = positionGraph.getVertices().filter(function(v) { return !v.isOuter(); });
	var componentCount = 0;	
	var defaultLength = ms.graph.DEFAULT_LENGTH;
	var min = new ms.vec2(Infinity, Infinity);
	var max = new ms.vec2(-Infinity, -Infinity);
	var updateMinMax = function(position) {
		min.x = Math.min(min.x, position.x);
		min.y = Math.min(min.y, position.y);
		max.x = Math.max(max.x, position.x);
		max.y = Math.max(max.y, position.y);
	};
	var componentVector = new ms.vec2(0, 0);
	while (verticesToAdd.length > 0) {
		var startVertex = verticesToAdd.shift();
		startVertex.core.position = componentVector.copy();
		var updateEndpoints = startVertex.getEndpoints().slice();
		updateMinMax(startVertex.core.position);
		while (updateEndpoints.length > 0) {
			var endpoint = updateEndpoints.shift();
			var prevVertex = endpoint.getVertex();
			var nextVertex = endpoint.twin().getVertex();
			
			// TODO: Handle cycles properly.
			if (nextVertex.getCore().position) {
				continue;
			}
			var baseLength = defaultLength;
			if (endpoint.getVertex().getEndpoints().length == 1 ||
			    endpoint.twin().getVertex().getEndpoints().length == 1) {
				baseLength = ms.graph.LEAF_LENGTH;
			}			
			var angle = endpoint.getAngle();
			var prevPosition = prevVertex.getCore().position.copy();
			var nextPosition = prevPosition.add(ms.vec2.unitVec(angle).scale((1 + 0.5 * (0.5 - ms.util.originalRandom())) * baseLength));
			nextVertex.core.position = nextPosition;
			ms.remove(nextVertex, verticesToAdd);
			var newEndpoints = nextVertex.getEndpoints().slice();
			newEndpoints = newEndpoints.filter(function(endpointB) {
				return endpoint.edge != endpointB.edge;
			});
			updateEndpoints = updateEndpoints.concat(newEndpoints);
			updateMinMax(nextPosition);
		}
		// Use the original random number generator to avoid changing the random number generator behavior during debugging.
		componentVector.x += ms.util.originalRandom() * ms.graph.COMPONENT_DISTANCE;
		componentVector.y += ms.util.originalRandom() * ms.graph.COMPONENT_DISTANCE;
	}
	
	var center = new ms.vec2((min.x + max.x) / 2, (min.y + max.y) / 2);
	var extents = new ms.vec2(max.x - min.x, max.y - min.y);
	var scale = Math.min(   rectExtents.x / extents.x, rectExtents.y / extents.y) * (1 - ms.familyTree.MARGIN);

	var context = view.getContext();
	context.lineWidth = ms.graph.FACE_EDGE_WIDTH;
	positionGraph.getEdges().forEach(function(edge) {
		var leftArea = edge.getCore().getLeftArea && edge.getCore().getLeftArea();
		if (leftArea) {
			ms.graph.drawEdge(edge, context, view, leftArea.getColor(), scale, center, rectCenter, 1);
		}
	});
	positionGraph.getEdges().forEach(function(edge) {
		var rightArea = edge.getCore().getRightArea && edge.getCore().getRightArea();
		if (rightArea) {
			ms.graph.drawEdge(edge, context, view, rightArea.getColor(), scale, center, rectCenter, -1);
		}
	});

	context.lineWidth = lineWidth;
	positionGraph.getEdges().forEach(function(edge) {
		var brush = edge.getCore().getBrush();
		if (brush && !options.colorOverride) {
			color = brush.getColor();
		}
		ms.graph.drawEdge(edge, context, view, color, scale, center, rectCenter, 0);
	});

	var convertToScreen = function(v) {
		return new ms.vec2(scale * (v.x - center.x) + rectCenter.x, scale * (center.y - v.y) + rectCenter.y);
	};
	positionGraph.getVertices().forEach(function(vertex) {
		if (vertex.getEndpoints().length == 1) {
			var position0 = vertex.getCore().position;
			var screenPos = convertToScreen(position0);
			var brush = vertex.getEndpoints()[0].getEdge().getCore().getBrush();
			var color = brush ? brush.getColor() : '#1d3c6b';
			view.context.strokeStyle = color;
			if (vertex.getCore().isOuter) {
				view.context.fillStyle = '#fff';
				view.drawCircleOld(screenPos, ms.vec2.ORIGIN, 4);
			} else {
				view.drawPoint(screenPos, color);
			}
		}
	});

	var highlightedEndpoints = options.endpoints || [];
	for (var i = 0; i < highlightedEndpoints.length; i++) {
		var endpoint = highlightedEndpoints[i];
		var index = this.edges.indexOf(endpoint.getEdge());
		var edge = positionEdges[index];
		var start = edge.getVertex(endpoint.getEdgeIndex()).core.position;
		var end = edge.getVertex(1 - endpoint.getEdgeIndex()).core.position;
		var angle = ms.vec2.angle(start, end);
		var position = view.convertToScreen(convertToScreen(start)).add(new ms.vec2(ms.shapeView.OFFSET_X, ms.shapeView.OFFSET_Y));
		ms.endpoint.drawArrow(context, position, angle, '#484');
	}
};

ms.graph.prototype.drawPretty = function(view, options) {
	var color = options.color || '#500';
	var rect = options.rect;
	var lineWidth = options.lineWidth || 2;

	// Don't use the outer vertex. It's not connected to anything.
	var verticesToAdd = this.getVertices().filter(function(v) { return !v.isOuter(); });
	var componentCount = 0;	
	var defaultLength = ms.graph.DEFAULT_LENGTH;
	var min = new ms.vec2(Infinity, Infinity);
	var max = new ms.vec2(-Infinity, -Infinity);
	var updateMinMax = function(position) {
		min.x = Math.min(min.x, position.x);
		min.y = Math.min(min.y, position.y);
		max.x = Math.max(max.x, position.x);
		max.y = Math.max(max.y, position.y);
	};
	verticesToAdd.forEach(function(v) {
		updateMinMax(v.getCore().position);
	});
	
	var rectCenter = new ms.vec2((rect[0] + rect[1]) / 2, (rect[2] + rect[3]) / 2);
	var rectExtents = new ms.vec2((rect[1] - rect[0]), (rect[3] - rect[2]));
	var center = new ms.vec2((min.x + max.x) / 2, (min.y + max.y) / 2);
	var extents = new ms.vec2(max.x - min.x, max.y - min.y);
	var scale = Math.min(rectExtents.x / extents.x, rectExtents.y / extents.y) * (1 - ms.familyTree.MARGIN);

	var context = view.getContext();
	context.lineWidth = lineWidth;
	this.getEdges().forEach(function(edge) {
		var brush = edge.getCore().getBrush();
		if (brush) {
			color = brush.getColor();
		}
		var position0 = edge.getVertex(0).getCore().position;
		var position1 = edge.getVertex(1).getCore().position;
		// Note that the y-axis is reversed.		
		var startPos = new ms.vec2(scale * (position0.x - center.x) + rectCenter.x, scale * (center.y - position0.y) + rectCenter.y);
		var endPos =   new ms.vec2(scale * (position1.x - center.x) + rectCenter.x, scale * (center.y - position1.y) + rectCenter.y);
		context.beginPath();
		var lineOptions = { hasArrows: [false, false] };
		view.drawLine(color, startPos, endPos, new ms.vec2(0, 0), lineOptions);
		context.stroke();
	});

	var convertToScreen = function(v) {
		return new ms.vec2(scale * (v.x - center.x) + rectCenter.x, scale * (center.y - v.y) + rectCenter.y);
	};
	
	// For leaves add a circle for outer endpoints add the index as text.
	context.fillStyle = '#888';
	context.font = '10px Arial';
	context.textAlign = 'center';
	this.getVertices().forEach(function(vertex) {
		// The outer vertex has no endpoints and causes errors if used.
		if (vertex.getEndpoints().length == 0) {
			return;
		}
		var outerEndpointIndex = vertex.getCore().outerEndpointIndex;
		var position0 = vertex.getCore().position;
		var screenPos = convertToScreen(position0);
		if (outerEndpointIndex >= 0) {
			view.drawText(screenPos, outerEndpointIndex, color);
		} else if (vertex.getEndpoints().length == 1) {
			// This needs to use the new 3D convertToScreen.
			view.drawPoint(screenPos, color);
		}
	});

	var highlightedEndpoints = options.endpoints || [];
	for (var i = 0; i < highlightedEndpoints.length; i++) {
		var endpoint = highlightedEndpoints[i];
		var index = this.edges.indexOf(endpoint.getEdge());
		var edge = positionEdges[index];
		var start = edge.getVertex(endpoint.getEdgeIndex()).core.position;
		var end = edge.getVertex(1 - endpoint.getEdgeIndex()).core.position;
		var angle = ms.vec2.angle(start, end);
		var position = view.convertToScreen(convertToScreen(start)).add(new ms.vec2(ms.shapeView.OFFSET_X, ms.shapeView.OFFSET_Y));
		ms.endpoint.drawArrow(context, position, angle, '#484');
	}
};

// This is a dummy class that takes the place of a vertex.
ms.outerVertex = function() {};

// Endpoints are kept in the order in which they are added.
ms.outerVertex.prototype.compare = function(endpointA, endpointB) {
	return false;
};