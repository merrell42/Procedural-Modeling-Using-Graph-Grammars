ms.node = function(element, stats, name, properties) {
	this.name = name;
	this.element = element;
	this.id = ms.node.count++;
	this.stats = stats;
	this.stats.addToChangeList(this);
	this.stats.addNode(this);
	this.destroyed = false;
	this.wasDestroyed = true;
	this.changed = true;
	this.destroyHandler = ms.nullFunction;
	this.undestroyHandler = ms.nullFunction;
	this.properties = properties;
	const values = Object.values(properties);
	for (var prop of values) {
		prop.setParent(this);
	}
	if (this.id == ms.globalSettings.get('Node Debug')) {
		ms.highlightedElement = this.element;
		debugger;
	}
};

ms.node.count = 0;

ms.node.prototype.getName = function() {
	return this.name;	
};

ms.node.prototype.getId = function() {
	return this.id;
};

ms.node.prototype.getStats = function() {
	return this.stats;
};

ms.node.prototype.getModel = function() {
	return this.stats.getModel();
};

ms.node.prototype.onChanged = function() {
	if (!this.changed) {
		this.changed = true;
		if (this.stats) {
			this.stats.addToChangeList(this);
		}
	}
};

ms.node.prototype.has = function(name) {
	return this.properties.hasOwnProperty(name);
};

/**
 * This can throw an error is a property is missing. Properties go missing
 * when rejecting nodes in the optimizer and then running restoreCreated.
 */
ms.node.prototype.get = function(name) {
	return this.properties[name].get();
};

ms.node.prototype.add = function(name, element, opt_atStart) {
	this.properties[name].add(element, opt_atStart);
};

ms.node.prototype.remove = function(name, element) {
	this.properties[name].remove(element);
};

ms.node.prototype.setChangeHandler = function(name, changeHandler) {
	this.properties[name].setChangeHandler(changeHandler);
};

ms.node.prototype.setValue = function(name, value) {
	var prop = this.properties[name];
	if (!(prop instanceof ms.valueProperty)) {
		ms.alert('setValue not called on a valueProperty.');
	}
	prop.set(value);
	this.onChanged();
};

// Sets the array.
ms.node.prototype.setArray = function(elementsB) {
	var nameA = this.name;
	var nameB = elementsB[0].getNode().getName();

	var self = this;
	elementsB.forEach(function(elementB) {
		var nodeB = elementB.getNode();
	
		// self.add(nameB, elementB);
		nodeB.add(nameA, self.element);
	});

	// var array = this.getArray(nameB);
	/* var removedElements = [];
	if (array.length > elementsB.length) {
		removedElements = array.filter(function(element) {
			return !elementsB.includes(element);
		});
		removedElements.forEach(function(element) {
			self.disconnect(element);
		});
	} */

	this.properties[nameB].setOrder(elementsB);
	// return removedElements;
};

// TODO: Clean up all these slightly different functions.
// ElementB is spliced into this property. This one is inserted into the element's property.
ms.node.prototype.spliceInsert = function(indexA, indexB, elementB) {
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.properties[nameB].splice(elementB, indexB);
	nodeB.properties[nameA].insert(this.element, indexA);
};

ms.node.prototype.doubleInsert = function(indexA, indexB, elementB) {
	if (indexA == -1 || indexB == -1) {
		debugger;
	}
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.properties[nameB].insert(elementB, indexB);
	nodeB.properties[nameA].insert(this.element, indexA);
};

ms.node.prototype.insert = function(elementB, index) {
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.properties[nameB].insert(elementB, index);
	nodeB.add(nameA, this.element);
};

ms.node.prototype.splice = function(elementB, index) {
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.properties[nameB].splice(elementB, index);
	nodeB.add(nameA, this.element);
};

// atStart is true if we are inserting the node at the beginning instead of the end.
ms.node.prototype.connect = function(elementB, opt_atStart) {
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.add(nameB, elementB, opt_atStart);
	nodeB.add(nameA, this.element, opt_atStart);
};

ms.node.prototype.disconnect = function(elementB) {
	var nodeB = elementB.getNode();
	var nameA = this.name;
	var nameB = nodeB.getName();

	this.remove(nameB, elementB);
	nodeB.remove(nameA, this.element);
};

ms.node.prototype.getElement = function() {
	return this.element;
};

ms.node.prototype.forEachNeighbor = function(func) {
	for (var prop in this.properties) {
		this.properties[prop].forEachNeighbor(func);
	}
};

ms.node.prototype.onNodeDestroyed = function(node) {
	if (this.destroyed) {
		return;
	}
	var name = node.getName();
	if (this.properties[name].onNodeDestroyed(node.getElement())) {
		this.destroy_();
	}
};

ms.node.prototype.destroy = function() {
	ms.timerG.start('Destroy');
	this.destroy_();
	ms.timerG.stop('Destroy');
};

ms.node.prototype.destroy_ = function() {
	if (this.destroyed) {
		return;
	}
	this.stats && this.stats.removeNode(this);
	this.destroyed = true;
	var self = this;
	this.forEachNeighbor(function(neighbor) {
		neighbor.getNode().onNodeDestroyed(self);
	});
	for (var prop in this.properties) {
		this.properties[prop].destroy();
	}
	
	this.destroyHandler();
	this.onChanged();
};

ms.node.prototype.setDestroyHandler = function(destroyHandler) {
	this.destroyHandler = destroyHandler;
};

ms.node.prototype.setUndestroyHandler = function(undestroyHandler) {
	this.undestroyHandler = undestroyHandler;
};

ms.node.prototype.isDestroyed = function() {
	return this.destroyed;
};

ms.node.prototype.save = function() {
	if (this.destroyed) {
		return;
	}
	this.changed = false;
	this.wasDestroyed = this.destroyed;
	for (var prop in this.properties) {
		this.properties[prop].save();
	}
};

ms.node.prototype.restoreDestroyed = function() {
	this.changed = false;
	var undestroyed = false;
	if (this.wasDestroyed) {
		this.properties = {};
		return;
	} else if (this.destroyed) {
		this.destroyed = false;
		undestroyed = true;
		this.stats.addNode(this);
	}
	for (var prop in this.properties) {
		this.properties[prop].restore();
	}
	if (undestroyed) {
		this.undestroyHandler();
	}
};

// Restore created nodes to back to being destroyed.
ms.node.prototype.restoreCreated = function() {
	if (this.wasDestroyed && !this.destroyed) {
		this.properties = {};
		this.destroy();
		return;
	}
};

ms.node.prototype.print = function() {
	this.element.print();
};
