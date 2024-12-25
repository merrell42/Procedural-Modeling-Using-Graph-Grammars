ms.graphEndpoint = function(vertex, edge) {
	this.vertex = vertex;
	this.edge = edge;
	this.edgeIndex = -1;
	this.vertexIndex = -1;
	
	this.nextOnFaceCached = {};
};

ms.graphEndpoint.prototype.export = function() {
	var graph = this.getGraph();
	return {
		vertex: graph.getVertices().indexOf(this.vertex),
		edge: graph.getEdges().indexOf(this.edge),
		vertexIndex: this.vertexIndex,
		edgeIndex: this.edgeIndex,
	}
};


ms.graphEndpoint.import = function(json, graph) {
	var vertex = graph.vertices[json.vertex];
	var edge = graph.edges[json.edge];
	var endpoint = new ms.graphEndpoint(vertex, edge);
	endpoint.vertexIndex = json.vertexIndex;
	endpoint.edgeIndex = json.edgeIndex;
	vertex.endpoints[json.vertexIndex] = endpoint;
	edge.endpoints[json.edgeIndex] = endpoint;
	return endpoint;
};

ms.graphEndpoint.prototype.getVertex = function() {
	return this.vertex;
};

ms.graphEndpoint.prototype.getEdge = function() {
	return this.edge;
};

ms.graphEndpoint.prototype.getGraph = function() {
	return this.vertex.getGraph();
};

ms.graphEndpoint.prototype.setEdgeIndex = function(edgeIndex) {
	this.edgeIndex = edgeIndex;
};

ms.graphEndpoint.prototype.setVertexIndex = function(vertexIndex) {
	this.vertexIndex = vertexIndex;
};

ms.graphEndpoint.prototype.getEdgeIndex = function() {
	return this.edgeIndex;
};

ms.graphEndpoint.prototype.getVertexIndex = function() {
	return this.vertexIndex;
};

ms.graphEndpoint.prototype.getIsAtStart = function() {
	return this.edgeIndex == 0;
};

ms.graphEndpoint.prototype.twin = function() {
	return this.edge.getEndpoint(1 - this.edgeIndex);
};

ms.graphEndpoint.prototype.nextOnFace = function(faceType) {
	var id = faceType.id;
	if (!this.nextOnFaceCached[id]) {
		var twin = this.twin();
		var angle0 = faceType.angle(twin.getDir());
		var nextVertex = twin.getVertex();
		// Find all endpoints with the same face type.
		var endpoints = nextVertex.getEndpoints();
		if (nextVertex.isOuter()) {
			return null;
		}
		endpoints = endpoints.filter(function(endpoint) {
			if (endpoint == twin) {
				return false;
			}
			return endpoint.getEdge().getCore().getFaceData().find(function(faceDatum) {
				return faceDatum.type == faceType;
			});
		});
		// If there is only one endpoint, pick it.
		if (endpoints.length == 1) {
			this.nextOnFaceCached[id] = endpoints[0];
		} else {
			var angles = endpoints.map(function(endpoint) {
				return faceType.angle(endpoint.getDir());
			});
			// Find the maximum angle that is less than angle0.
			for (var i = 0; i < angles.length; i++) {
				if (angles[i] >= angle0) {
					angles[i] -= 2 * Math.PI;
				}
			}
			var maxAngle = angles[0]
			var maxIndex = 0;
			for (var i = 1; i < angles.length; i++) {
				if (angles[i] > maxAngle) {
					maxAngle = angles[i];
					maxIndex = i;
				}
			}
			this.nextOnFaceCached[id] = endpoints[maxIndex];
		}
	}
	return this.nextOnFaceCached[id];
};

ms.graphEndpoint.prototype.next = function() {
	var twin = this.twin();
	return twin && twin.clockwise();
};

ms.graphEndpoint.prototype.prev = function() {
	var counter = this.counter();
	return counter && counter.twin();
};

ms.graphEndpoint.prototype.clockwise = function() {
	/* if (this.vertex.isOuter()) {
		return null;
	} */
	var endpoints = this.vertex.getEndpoints();
	var nextIndex = this.vertexIndex > 0 ? this.vertexIndex - 1 : endpoints.length - 1;	
	return endpoints[nextIndex];
};

ms.graphEndpoint.prototype.counter = function() {
	/* if (this.vertex.isOuter()) {
		return null;
	} */
	var endpoints = this.vertex.getEndpoints();
	var prevIndex = (this.vertexIndex + 1) % endpoints.length;	
	return endpoints[prevIndex];
};

ms.graphEndpoint.prototype.getCore = function() {
	return this.edge.getCore().getEndpoints()[this.edgeIndex];
};

ms.graphEndpoint.prototype.getAngle = function() {
	return ms.util.fixAngle(this.edge.getCore().getAngle() + this.edgeIndex * Math.PI);
};

// Get the angle with respect to a particular face normal.
ms.graphEndpoint.prototype.getFaceAngle = function(faceType) {
	return faceType.angle(this.getDir());
};

ms.graphEndpoint.prototype.getDir = function() {
	var dir = this.edge.getCore().getDir().copy();
	if (this.edgeIndex == 1) {
		return dir.scale(-1);
	} else {
		return dir;
	}
};

ms.graphEndpoint.prototype.id = function() {
	return {edge: this.edge.getCore().id, dir: this.edgeIndex};
};

ms.graphEndpoint.prototype.signature = function() {
	return this.edge.getCore().id + '.' + this.edgeIndex;
};

ms.graphEndpoint.prototype.connectionId = function() {
	return this.getGraph().id + ',' + this.vertexIndex;
};

ms.graphEndpoint.prototype.getFaceData = function() {
	var faceData = this.edge.getCore().getFaceData();
	var expectLeft = (this.edgeIndex == 0);
	return faceData.filter(function(datum) {
		return datum.onRight == expectLeft;
	});
};

ms.graphEndpoint.prototype.opposite = function() {
	var endpoints = this.getGraph().getOuterEndpoints();
	if (endpoints.length != 2) {
		ms.alert('Opposite requires graph with two outer endpoints.');
	}
	var index = endpoints.indexOf(this);
	return endpoints[1 - index];
};

ms.graphEndpoint.prototype.idOld = function() {
	return 2 * this.edge.getCore().id + this.edgeIndex;
};

ms.graphEndpoint.prototype.copy = function() {
	var index = this.outerIndex();
	var graph = this.getGraph().copy();
	return graph.getOuterEndpoints()[index];
};

ms.graphEndpoint.prototype.outerIndex = function() {
	return this.getGraph().getOuterEndpoints().indexOf(this);
};

ms.graphEndpoint.prototype.faceTypes = function() {
	return this.edge.core.faceData.map(function(datum) { return datum.type; });
};

// Creates a new graph with endpointA attached to endpointB.
// Return the newGraph. aDest is where the endpoints in graphA ended up.
// newSource is where the endpoints in the new graph came from.
ms.graphEndpoint.attachEndpoints = function(endpointA, endpointB) {
	var graphA = endpointA.getGraph();
	var graphB = endpointB.getGraph();
	var endpointsA = graphA.getOuterEndpoints();
	var endpointsB = graphB.getOuterEndpoints();

	var copyA = endpointA.copy();
	if (graphA == graphB) {
		var copyB = copyA.getGraph().getOuterEndpoints()[endpointB.outerIndex()];
	} else {
		var copyB = endpointB.copy();
	}
	var aDest = [];
	var bDest = [];
	var newSource = [];
	var newGraph = copyA.getGraph();
	var newEndpoints = newGraph.getOuterEndpoints();
	for (var i = 0; i < endpointsA.length; i++) {
		aDest[i] = newEndpoints[i];
		newSource[i] = endpointsA[i];
	}

	var mergedB = copyB;
	if (graphA == graphB) {
		bDest = aDest;
	} else {
		var numEndpointsA = graphA.getOuterEndpoints().length;
		var numEndpointsB = graphB.getOuterEndpoints().length;
		var outerIndexB = copyB.outerIndex();
		newGraph.merge(copyB.getGraph());
		newEndpoints = newGraph.getOuterEndpoints();
		for (var i = 0; i < numEndpointsB; i++) {
			bDest[i] = newEndpoints[numEndpointsA + i];
			newSource[numEndpointsA + i] = endpointsB[i];
		}
		mergedB = newEndpoints[numEndpointsA + outerIndexB];
	}
	var edgeA = copyA.getEdge();
	var edgeB = mergedB.getEdge();
	var vertexA = copyA.twin().getVertex();
	var vertexB = mergedB.twin().getVertex();
	var aIsFirst = (copyA.twin().edgeIndex == 0);
	var vertex0 = aIsFirst ? vertexA : vertexB;
	var vertex1 = aIsFirst ? vertexB : vertexA;

	// Remove A.
	var indexA = copyA.outerIndex();
	newGraph.removeEdge(edgeA);
	aDest[endpointA.outerIndex()] = null;
	newSource.splice(indexA, 1);

	// Remove B.
	var indexB = mergedB.outerIndex();
	newGraph.removeEdge(edgeB);
	bDest[endpointB.outerIndex()] = null;
	newSource.splice(indexB, 1);

	// Add edge between them.
	newGraph.addEdge(edgeA, vertex0, vertex1);
	return {newGraph: newGraph, aDest: aDest, bDest: bDest, newSource: newSource};
};

ms.graphEndpoint.prototype.attachEndpoint = function(endpointB) {
	var graphA = this.getGraph();
	var graphB = endpointB.getGraph();
	var oldToNewEndpoints = [];
	var oldGraph = null;
	var mergedB = endpointB;

	if (graphA != graphB) {
		var numEndpointsA = graphA.getOuterEndpoints().length;
		var numEndpointsB = graphB.getOuterEndpoints().length;
		var outerIndexB = endpointB.outerIndex();
		graphA.merge(endpointB.getGraph());

		for (var i = 0; i < numEndpointsB; i++) {
			oldToNewEndpoints.push({
				old: graphB.getOuterEndpoints()[i],
				new: graphA.getOuterEndpoints()[numEndpointsA + i]
			});
		}
		mergedB = graphA.getOuterEndpoints()[numEndpointsA + outerIndexB];
		oldGraph = graphB;
	}
	
	var edgeA = this.getEdge();
	var edgeB = mergedB.getEdge();
	var vertexA = this.twin().getVertex();
	var vertexB = mergedB.twin().getVertex();
	var aIsFirst = (this.twin().edgeIndex == 0);
	var vertex0 = aIsFirst ? vertexA : vertexB;
	var vertex1 = aIsFirst ? vertexB : vertexA;

	graphA.removeEdge(edgeA);
	graphA.removeEdge(edgeB);
	graphA.addEdge(edgeA, vertex0, vertex1);
	return {oldToNewEndpoints: oldToNewEndpoints, oldGraph: oldGraph};
};

ms.graphEndpoint.prototype.windingNumber = function() {
	return ms.graph.windingNumber(this);
};

ms.graphEndpoint.prototype.equals = function(vertexB) {
	if (this.edgeIndex != vertexB.edgeIndex || 
		this.vertexIndex != vertexB.vertexIndex) {
		return false;
	} else {
		return true;
	}
};

ms.graphEndpoint.prototype.highlight = function(view) {
	var options = {endpoints: [this]};
	this.vertex.getGraph().highlight(view, options);
}

ms.graphEndpoint.prototype.requiresShapeView = function () {
	return true;
};

// Returns true if the endpoint is oriented correctly around the face. That is the
// face is on the left side of the endpoint and the endpoint is winding counter-
// clockwise around the face.
// Should match ms.endpoint.oriented.
ms.graphEndpoint.prototype.oriented = function(faceType) {
	var faceData = this.getEdge().getCore().getFaceData();
	var faceDatum = faceData.find(function(datum) { return datum.type == faceType; });
	if (!faceDatum) {
		ms.alert('Face type is not found on endpoint\'s edge ');
		return false;
	}
	return !!(faceDatum.onRight ^ this.getEdgeIndex())
};

// Return the face type that is oriented.
ms.graphEndpoint.prototype.orientedFaceType = function() {
	var faceData = this.getEdge().getCore().getFaceData();
	var self = this;
	var faceDatum = faceData.find(function(datum) { return !!(datum.onRight ^ self.getEdgeIndex()) });
	return faceDatum.type;
};

// Order does not matter for graph shells that use graph endpoints as vertices.
ms.graphEndpoint.prototype.compare = function(endpointA, endpointB) {
	return false;
};

ms.graphEndpoint.prototype.print = function() {
	ms.highlight(this);
	return this.getAngle();
};

// This is useful for 3D graphs.
ms.graphEndpoint.prototype.printV = function() {
	this.vertex.core.print();
};
