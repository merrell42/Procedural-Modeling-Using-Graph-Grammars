ms.transistorPath = function(indices, lines) {
	this.indices = indices;
	this.lines = lines;
	this.endpoints = [];
	this.extendable = [true, true];
	this.id = ms.transistorPath.count++;
};

ms.transistorPath.count = 0;

ms.transistorPath.createNet = function(endpoints, edges, lines) {
	var indices = [];
	for (var i = 0; i < endpoints.length; i++) {
		var endpoint = endpoints[i];
		var index = edges.indexOf(endpoint.getLine());
		if (index == -1) {
			ms.alert('endpoint is not found in edges.');
		}
		indices.push({index: index, isForward: endpoint.isAtStart == 0});
	}
	return new ms.transistorPath(indices, lines);
};

ms.transistorPath.create = function(endpoints, edges, lines) {
	var indices = [];
	for (var i = 0; i < endpoints.length; i++) {
		var endpoint = endpoints[i];
		var index = edges.indexOf(endpoint.getEdge());
		if (index == -1) {
			ms.alert('endpoint is not found in edges.');
		}
		indices.push({index: index, isForward: endpoint.getEdgeIndex() == 0});
	}
	return new ms.transistorPath(indices, lines);
};

ms.transistorPath.prototype.setEndpoints = function(endpoints) {
	this.endpoints = endpoints;
};

ms.transistorPath.prototype.extendableness = function() {
	return this.extendable[0] + this.extendable[1];
};

ms.transistorPath.prototype.randomNextVertex = function() {
	var probabilities = this.extendable.map(function(e) { return e ? 1 : 0; });
	var index = ms.randomDistribution(probabilities);
	return this.endpoints[index].getVertex();
};

ms.transistorPath.prototype.rigidNextVertex = function() {
	for (var i = 0; i < 2; i++){
		if (this.extendable[i] && this.indices.length >= 2) {
			var iIndices;
			if (i == 0) {
				iIndices = [0, 1];
			} else {
				iIndices = [this.indices.length - 2, this.indices.length - 1];
			}
			// Two consecutive indices must be rigid.
			var rigid = true;
			for (var j = 0; j < 2; j++) {
				var line = this.lineFromIndex(iIndices[i]);
				rigid = rigid && !line.getEdgeType().extendable();
			}
			if (rigid) {
				return this.endpoints[i].getVertex();
			}
		}
	}
	return null;
};

ms.transistorPath.prototype.lineFromIndex = function(index) {
	return this.lines[this.indices[index].index];
};

ms.transistorPath.prototype.indexForEndpoint = function(endpoint) {
	return {
		index: this.lines.indexOf(endpoint.getLine()),
		isForward: endpoint.getIsAtStart()
	};
};

ms.transistorPath.prototype.expandBackward = function() {
	var prevEndpoint = this.endpoints[0].prev();
	if (prevEndpoint) {
		this.endpoints[0] = prevEndpoint;
		this.indices.unshift(this.indexForEndpoint(prevEndpoint));
	} else {
		this.extendable[0] = false;
	}
};

ms.transistorPath.prototype.expandForward = function() {
	this.indices.push(this.indexForEndpoint(this.endpoints[1]));
	var nextEndpoint = this.endpoints[1].next();
	if (nextEndpoint) {
		this.endpoints[1] = nextEndpoint;
	} else {
		this.extendable[1] = false;
	}
};

ms.transistorPath.prototype.merge = function(pathB) {
	this.endpoints[1] = pathB.endpoints[1];
	this.extendable[1] = pathB.extendable[1];
	this.indices = this.indices.concat(pathB.indices);
};

ms.transistorPath.prototype.highlight = function(context, convertToScreen) {
	for (var i = 0; i < this.endpoints.length; i++) {
		this.endpoints[i].highlight(context, convertToScreen);
	}
	for (var i = 0; i < this.indices.length; i++) {
		this.lineFromIndex(i).highlight(context, convertToScreen);
	}
};

ms.transistorPath.prototype.print = function() {
	ms.highlight(this);
};