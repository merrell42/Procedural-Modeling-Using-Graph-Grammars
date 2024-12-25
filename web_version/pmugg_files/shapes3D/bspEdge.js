// Connects a parent BSP node to a child BSP node.
// This wouldn't be needed if we could connect the nodes directly.
ms.bspEdge = function(stats) {
	var properties = {
		bspNode: new ms.requiredArray('bspNode', false, 2),
	};
	this.node = new ms.node(this, stats, 'bspEdge', properties);
	this.node.setChangeHandler('bspNode', this.onChanged.bind(this));
};

ms.bspEdge.prototype.addNodes = function(parentNode, childNode, isAbove) {
	// The child must be added first. If the parent comes first, onChange will delete this.
	this.node.doubleInsert(0, 1, childNode);
	this.node.doubleInsert(isAbove ? 1 : 2, 0, parentNode);
};

ms.bspEdge.prototype.getNode = function() {
	return this.node;
};

ms.bspEdge.prototype.getChild = function() {
	return this.node.get('bspNode')[1];
};

ms.bspEdge.prototype.onChanged = function() {
	if (!this.getChild()) {
		this.node.destroy();	
	}
};
