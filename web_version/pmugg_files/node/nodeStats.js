ms.nodeStats = function() {
	this.nodes = {};
	this.count = {};
	for (var i = 0; i < ms.nodeStats.nodeTypes.length; i++) {
		var name = ms.nodeStats.nodeTypes[i];
		this.nodes[name] = {};
		this.count[name] = 0;
	}
	this.changeList = [];
	this.costChange = {};
	this.model = null;
	this.resetCostChange();
	this.badCollection = new ms.collection(this, 'vertex');
	this.unloopedCollection = new ms.collection(this, 'face');
	this.badCollection.getNode().save();
	this.unloopedCollection.getNode().save();
};

ms.nodeStats.nodeTypes = [
	'cell', 'collection', 'endpoint', 'face', 'faceConnection', 'faceGroup', 'line', 'lineSegment', 'lineState', 'vertex', 'vertexState',
	'ringInstance', 'bspNode', 'bspEdge', 'bspPolygon',
];

ms.nodeStats.costTerms = ['lineDistance', 'valence', 'reject'];

ms.nodeStats.prototype.setModel = function(model) {
	this.model = model;
};

ms.nodeStats.prototype.getModel = function(node) {
	return this.model;
}

ms.nodeStats.prototype.addNode = function(node) {
	var name = node.getName();
	this.nodes[name][node.getId()] = node;
	this.count[name]++;
};

ms.nodeStats.prototype.removeNode = function(node) {
	var name = node.getName();
	delete this.nodes[name][node.getId()];
	this.count[name]--;
};

ms.nodeStats.prototype.addVertex = function(node) {
	var vertex = node.element;
	if (vertex.hasConflict()) {
		node.connect(this.badCollection);
	} else {
		if (node.has('collection') && node.get('collection')) {
			node.disconnect(this.badCollection);
		}
	}
};

ms.nodeStats.prototype.removeVertex = function(node) {
	if (node.has('collection') && node.get('collection')) {
		node.disconnect(this.badCollection);
	}
};

ms.nodeStats.prototype.updateFace = function(face, unlooped) {
	var node = face.getNode();
	if (unlooped) {
		node.connect(this.unloopedCollection);
	} else {
		if (node.has('collection') && node.get('collection')) {
			node.disconnect(this.unloopedCollection);
		}
	}
};

ms.nodeStats.prototype.getBadVertices = function() {
	return this.badCollection.getVertices();
};

ms.nodeStats.prototype.getUnloopedFaces = function() {
	return this.unloopedCollection.getFaces();
};

ms.nodeStats.prototype.getElements = function(name) {
	if (!this.nodes.hasOwnProperty(name)) {
		return [];
	}
	var elements = [];
	return Object.values(this.nodes[name]).map(function(node) {
		return node.getElement();
	});
};

ms.nodeStats.prototype.getCount = function(name) {
	return this.count.hasOwnProperty(name) ? this.count[name] : 0
};

ms.nodeStats.prototype.addToChangeList = function(node) {
	this.changeList.push(node);
};

ms.nodeStats.prototype.apply = function(func) {
	for (var type in this.nodes) {
		for (var id in this.nodes[type]) {
			func(this.nodes[type][id]);
		}
	}
};

ms.nodeStats.prototype.save = function() {
	this.changeList.forEach(function(node) {
		node.save();
	});
	this.changeList = [];
	this.resetCostChange();
};

ms.nodeStats.prototype.restore = function() {
	this.changeList.forEach(function(node) {
		node.restoreDestroyed();
	});
	this.changeList.forEach(function(node) {
		node.restoreCreated();
	});
	this.changeList = [];
	this.resetCostChange();
};

ms.nodeStats.get = function() {
	return window.controller.currentController.synthesizer.mutator.nodeStats;
};
	
// This only used for testing to verify nothing weird has happened.
ms.nodeStats.verifyAll = function() {
	ms.nodeStats.get().verifyAll();
};

ms.nodeStats.prototype.verifyAll = function() {
	for (var name in this.nodes) {
		for (var id in this.nodes[name]) {
			var node = this.nodes[name][id];
			if (node.isDestroyed()) {
				window.console.log('Destroyed node');
				debugger;
			}
		}
	}
	
	for (var id in this.nodes['lineState']) {
		var element = this.nodes['lineState'][id].element;
		if (!element.cell.activeStates.default.includes(element)) {
			window.console.log('Cell missing node.');
			debugger;
		}
	}
};

ms.nodeStats.prototype.componentCount = function() {
	var count = 0;
	var faces = this.nodes['face'];
	for (id in faces) {
		var face = faces[id];
		if (face.getElement().isHole()) {
			count++;
		}
	}
	return count;
};

ms.nodeStats.prototype.resetCostChange = function() {
	var self = this;
	ms.nodeStats.costTerms.forEach(function(costTerm) {
		self.costChange[costTerm] = 0;
	});
	if (ms.optimizer.detailedCost) {
		this.costChange.details = {};
	}
};
