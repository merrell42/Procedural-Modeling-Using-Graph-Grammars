ms.ringTreeNode = function(data) {
	this.parent = null;
	this.children = {};
	this.data = data;
};

ms.ringTreeNode.prototype.export = function(rings, vertexTypes) {
	var data = {...this.data};
	if (this.data.ring) {
		data.ring = rings.indexOf(this.data.ring);
	}
	if (this.data.vertex) {
		data.vertex = vertexTypes.indexOf(this.data.vertex);
	}
	var children = {};
	var result = { data, children};
	var self = this;
	Object.keys(this.children).forEach((key) => {
		children[key] = self.children[key].export(rings, vertexTypes);
	});
	return result;
};

ms.ringTreeNode.import = function(json, rings, types) {
	var result = new ms.ringTreeNode({...json.data});
	if (json.data.ring !== undefined) {
		result.data.ring = rings[json.data.ring];
	}
	if (json.data.vertex !== undefined) {
		result.data.vertex = types.vertexTypes[json.data.vertex];
	}
	Object.keys(json.children).forEach((key) => {
		var child = ms.ringTreeNode.import(json.children[key], rings, types);
		result.children[key] = child;
		child.parent = result;
	});
	return result;
};

ms.ringTreeNode.types = {
	ROOT: 0,
	START_EDGE: 1,
	CONNECTOR: 2,
	VERTEX: 3,
	LOOP: 4,
	END: 5,
};

ms.ringTreeNode.dataId = function(data) {
	var types = ms.ringTreeNode.types;
	switch(data.type) {
		case types.ROOT:
		case types.CONNECTOR:
			return data.type;
		case types.START_EDGE:
			return data.type + ',' + data.edgeId;
		case types.VERTEX:
			return data.type + ',' + data.vertex.id;
		case types.LOOP:
			return data.type + ',' + data.loopIndex;
		case types.END:
			return data.type;
	}
};


ms.ringTreeNode.prototype.getChild = function(data) {
	var id = ms.ringTreeNode.dataId(data);
	if (this.children[id]) {
		return this.children[id];
	} else {
		return null;
	}
};

ms.ringTreeNode.prototype.forceGetChild = function(data) {
	var id = ms.ringTreeNode.dataId(data);
	if (this.children[id]) {
		return this.children[id];
	}
	var child = new ms.ringTreeNode(data);
	this.children[id] = child;
	child.setParent(this);
	return child;
};

ms.ringTreeNode.prototype.setParent = function(parent) {
	this.parent = parent;	
};

ms.ringTreeNode.prototype.grow = function(endpoints, edgeSlots, ring) {
	var types = ms.ringTreeNode.types;
	if (endpoints.length == 0) {
		if (this.children.length > 0) {
			console.log('Duplicate rings in grow.');
		}
		this.forceGetChild({type: types.END, edgeSlots: edgeSlots, ring: ring });
		return;
	}
	var endpoint = endpoints.shift();
	var twin = endpoint.twin();	
	var vertex = twin.getVertex();
	var child = null;
	if (vertex.isOuter()) {
		child = this.forceGetChild({type: types.CONNECTOR});
	} else {
		var loopIndex = endpoints.indexOf(twin);
		if (loopIndex >= 0 && this.data.type != types.START_EDGE) {
			child = this.forceGetChild({type: types.LOOP, loopIndex: loopIndex});
			endpoints.splice(loopIndex, 1);
		} else {
			var newEndpoints = vertex.getEndpoints().filter(function(e) { return e != twin; });
			endpoints = newEndpoints.concat(endpoints);
			child = this.forceGetChild({type: types.VERTEX, vertex: vertex.core });
		}
	}
	edgeSlots.push(ring.getGraph().getEdges().indexOf(endpoint.edge));
	child.grow(endpoints, edgeSlots, ring);
};

ms.ringTreeNode.prototype.traverse = function(endpoints, lineSlots) {
	var types = ms.ringTreeNode.types;
	var endpoint = null;
	var type = this.data.type;
	if (type != types.START_EDGE && type != types.END) {
		endpoint = endpoints.shift();
		lineSlots.push(endpoint.getLine());
	}	
	switch (type) {
		case types.START_EDGE:
		case types.CONNECTOR:
			break;
		case types.VERTEX:
			var twin = endpoint.getTwin();	
			var vertex = twin.getVertex();
			if (this.data.vertex != vertex.getState().getType()) {
				return [];
			}
			var newEndpoints = vertex.getEndpoints().filter(function(e) { return e != twin; });
			endpoints = newEndpoints.concat(endpoints);
			break;
		case types.LOOP:
			var loopEndpoints = endpoints.splice(this.data.loopIndex, 1);
			if (loopEndpoints[0] != endpoint.getTwin()) {
				return [];
			}
			break;
		case types.END:
			var stats = lineSlots[0].getNode().getStats();
			var instance = new ms.ringInstance(this.data.ring, stats);
			for (var i = 0; i < this.data.edgeSlots.length; i++) {
				var slot = this.data.edgeSlots[i];
				var currentSlot = instance.getLines()[slot];
				var newSlot = lineSlots[i];
				if (currentSlot && currentSlot != newSlot) {
					ms.alert('Slot mismatch');
				}
				instance.setLine(newSlot, slot);
			}
			// Check for lines that appear twice. They must both touch an outer vertex if they do.
			var lines = instance.getLines();
			for (var i = 0; i < lines.length; i++) {
				for (var j = i + 1; j < lines.length; j++) {
					if (lines[i] == lines[j]) {
						var edges = this.data.ring.graph.getEdges()
						var touchOutsideI = edges[i].getEndpoint(0).getVertex().isOuter() || 
						                    edges[i].getEndpoint(1).getVertex().isOuter();
						var touchOutsideJ = edges[j].getEndpoint(0).getVertex().isOuter() || 
						                    edges[j].getEndpoint(1).getVertex().isOuter();
						if (!touchOutsideI || !touchOutsideJ) {
							return [];
						}
					}
				}
			}
			return [instance];
	}
	var instances = [];
	var children = Object.values(this.children);
	for (var i = 0; i < children.length; i++) {
		instances = instances.concat(children[i].traverse(endpoints.slice(), lineSlots.slice()));
	}
	return instances;
};

ms.ringTreeNode.prototype.getRingInstances = function(line) {
	var types = ms.ringTreeNode.types;
	var child = this.getChild({type: types.START_EDGE, edgeId: line.getEdgeType().getId()});
	if (child) {
		return child.traverse(line.getEndpoints(), []);
	} else {
		return [];
	}
};

ms.ringTreeNode.createTree = function(rings) {
	var types = ms.ringTreeNode.types;
	var tree = new ms.ringTreeNode({type: types.ROOT});
	rings.forEach(function(ring) {
		var edges = ring.getGraph().getEdges();
		edges.forEach(function(edge) {
			var child = tree.forceGetChild({type: types.START_EDGE, edgeId: edge.core.id});
			var endpoints = [edge.getEndpoint(0), edge.getEndpoint(1)];
			child.grow(endpoints, [], ring);
		});
	});
	return tree;
};