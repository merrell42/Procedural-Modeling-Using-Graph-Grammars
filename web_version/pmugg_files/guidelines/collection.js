ms.collection = function(stats, type) {
	var properties = {};
	properties[type] = ms.unorderedAlternatives.enabled ? new ms.unorderedAlternatives(type) : new ms.alternativeArray(type);
	this.node = new ms.node(this, stats, 'collection', properties);
};

ms.collection.prototype.getNode = function() {
	return this.node;
};

ms.collection.prototype.getVertices = function() {
	if (this.node.isDestroyed()) {
		return [];
	} else {
		return this.node.get('vertex');
	}
};

ms.collection.prototype.getFaces = function() {
	if (this.node.isDestroyed()) {
		return [];
	} else {
		return this.node.get('face');
	}
};

ms.collection.prototype.getRingInstances = function() {
	if (this.node.isDestroyed()) {
		return [];
	} else {
		return this.node.get('ringInstance');
	}
};
