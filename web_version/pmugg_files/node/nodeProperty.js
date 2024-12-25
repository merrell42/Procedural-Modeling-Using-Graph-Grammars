ms.singleProperty = function(name, required) {
	this.name = name;
	this.required = required;
	this.element = null;
	this.savedElement = null;
	this.parent = null;
	this.changeHandler = function() {};
};

ms.singleProperty.prototype.setParent = function(parent) {
	this.parent = parent;
};

ms.singleProperty.prototype.getName = function() {
	return this.name;
};

ms.singleProperty.prototype.get = function() {
	return this.element;
};

ms.singleProperty.prototype.forEachNeighbor = function(func) {
	this.element && func(this.element);
};

ms.singleProperty.prototype.setChangeHandler = function(changeHandler) {
	this.changeHandler = changeHandler;
};

ms.singleProperty.prototype.onChanged = function() {
	this.parent.onChanged();
	this.changeHandler();
};

ms.singleProperty.prototype.destroy = function() {};

ms.singleProperty.prototype.add = function(element, opt_atStart) {
	if (this.element == element) {
		return;
	} else if (this.element) {
		this.element.getNode().disconnect(this.parent.getElement());
	}
	this.element = element;
	this.onChanged();
};

ms.singleProperty.prototype.remove = function(element) {
	if (this.element != element) {
		ms.alert('Removing an element that does not exist.');
	}
	this.element = null;
	this.onChanged();
};

ms.singleProperty.prototype.onNodeDestroyed = function(element) {
	this.remove(element);
	return this.required;
};

ms.singleProperty.prototype.save = function() {
	this.savedElement = this.element;
};

ms.singleProperty.prototype.restore = function() {
	this.element = this.savedElement;
};

// TODO: Rename requiredArray as it is not always required.
ms.requiredArray = function(name, required, opt_size) {
	this.name = name;
	this.required = required;
	this.size = opt_size || 0;
	this.array = [];
	this.savedArray = [];
	for (var i = 0; i < this.size; i++) {
		this.array.push(null);
	}
	this.parent = null;
	this.changeHandler = function() {};
};

ms.requiredArray.prototype.setParent = function(parent) {
	this.parent = parent;
};

ms.requiredArray.prototype.getName = function() {
	return this.name;
};

ms.requiredArray.prototype.get = function() {
	return this.array;
};

ms.requiredArray.prototype.forEachNeighbor = function(func) {
	return this.array.forEach(function(x) { x && func(x); });
};

ms.requiredArray.prototype.splice = function(element, index) {
	this.array.splice(index, 0, element);
	this.onChanged();
};

ms.requiredArray.prototype.insert = function(element, index) {
	if (index > this.size) {
		ms.alert('Index out of bound in required array.');
	}
	this.array[index] = element;
	this.onChanged();
};

ms.requiredArray.prototype.setChangeHandler = function(changeHandler) {
	this.changeHandler = changeHandler;
};

ms.requiredArray.prototype.onChanged = function() {
	this.parent.onChanged();
	this.changeHandler();
};

ms.requiredArray.prototype.destroy = function() {};

ms.requiredArray.prototype.add = function(element, opt_atStart) {
	if (this.size == 0) {
		if (opt_atStart) {
			this.array.unshift(element);
		} else {
			this.array.push(element);
		}
	} else {
		var index = 0;
		while (this.array[index]) {
			index++;
		}
		if (index >= this.size) {
			ms.alert('Out of bounds in required array.');
		}
		this.array[index] = element;
	}
	this.onChanged();
};

ms.requiredArray.prototype.remove = function(element) {
	if (this.size > 0) {
		var index = this.array.indexOf(element);
		if (index < 0) {
			ms.alert('Removing an element that does not exist');
		} else {
			this.array[index] = null;
		}
	} else {
		ms.remove(element, this.array);
	}
	this.onChanged();
};

// Returns whether or not the node should be destroyed after the
// element was destroyed.
ms.requiredArray.prototype.onNodeDestroyed = function(element) {
	this.remove(element);
	if (this.required) {
		for (var i = 0; i < this.array.length; i++) {
			if (this.array[i]) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
};

ms.requiredArray.prototype.save = function() {
	this.savedArray = this.array.slice();
};

ms.requiredArray.prototype.restore = function() {
	this.array = this.savedArray.slice();
};

ms.alternativeArray = function(name, required, opt_isOrdered) {
	this.name = name;
	this.required = required;
	this.array = [];
	this.savedArray = []
	this.parent = null;
	if (opt_isOrdered) {
		alert('isOrdered no longer supported');
	}
	// this.isOrdered = opt_isOrdered || false;
	this.changeHandler = function() {};
};

ms.alternativeArray.prototype.setParent = function(parent) {
	this.parent = parent;
};

ms.alternativeArray.prototype.getName = function() {
	return this.name;
};

ms.alternativeArray.prototype.get = function() {
	return this.array
};

ms.alternativeArray.prototype.forEachNeighbor = function(func) {
	return this.array.forEach(func);
};

ms.alternativeArray.prototype.setChangeHandler = function(changeHandler) {
	this.changeHandler = changeHandler;
};

ms.alternativeArray.prototype.onChanged = function() {
	this.parent.onChanged();
	this.changeHandler();
};

ms.alternativeArray.prototype.destroy = function() {};

ms.alternativeArray.prototype.splice = function(element, index) {
	this.array.splice(index, 0, element);
	this.onChanged();
};

ms.alternativeArray.prototype.add = function(element) {
	if (this.array.includes(element)) {
		return;
	}
	this.array.push(element);
	this.onChanged();
};

ms.alternativeArray.prototype.setOrder = function(array) {
	this.array = array;
};

ms.alternativeArray.prototype.remove = function(element) {
	ms.remove(element, this.array);
	this.onChanged();
};

ms.alternativeArray.prototype.onNodeDestroyed = function(element) {
	ms.remove(element, this.array);
	this.onChanged();
	return this.required && this.array.length == 0;
};

ms.alternativeArray.prototype.save = function() {
	this.savedArray = this.array.slice();
};

ms.alternativeArray.prototype.restore = function() {
	this.array = this.savedArray.slice();
};

// This is supposed to be a faster version of alternativeArray, but there isn't
// strong evidence it's faster probably because the array sizes are small. The
// main difference is that this node is not destroyed when the alternatives are
// empty.
ms.unorderedAlternatives = function(name) {
	this.name = name;
	this.list = {};
	this.savedList = {};
	this.parent = null;
	this.changeHandler = function() {};
};

// If this is disabled be sure to recreate the collection when it is destroyed.
ms.unorderedAlternatives.enabled = true;

ms.unorderedAlternatives.prototype.setParent = function(parent) {
	this.parent = parent;
};

ms.unorderedAlternatives.prototype.getName = function() {
	return this.name;
};

ms.unorderedAlternatives.prototype.get = function() {
	return Object.values(this.list);
};

ms.unorderedAlternatives.prototype.forEachNeighbor = function(func) {
	for (var key in this.list) {
		func(this.list[key]);
	}
};

ms.unorderedAlternatives.prototype.setChangeHandler = function(changeHandler) {
	this.changeHandler = changeHandler;
};

ms.unorderedAlternatives.prototype.onChanged = function() {
	this.parent.onChanged();
	this.changeHandler();
};

ms.unorderedAlternatives.prototype.destroy = function() {};

ms.unorderedAlternatives.prototype.splice = function(element, index) {
	alert('splice should not happen');
	// this.array.splice(index, 0, element);
	// this.onChanged();
};

ms.unorderedAlternatives.prototype.add = function(element) {
	var id = element.node.id;
	if (this.list.hasOwnProperty(id)) {
		return;
	}
	this.list[id] = element;
	this.onChanged();
};

ms.unorderedAlternatives.prototype.setOrder = function(array) {
	alert('setOrder should not happen');
	// this.array = array;
};

ms.unorderedAlternatives.prototype.remove = function(element) {
	// TODO: Maybe throw an error message if the node is not in the list.
	delete this.list[element.node.id];
	this.onChanged();
};

ms.unorderedAlternatives.prototype.onNodeDestroyed = function(element) {
	delete this.list[element.node.id];
	this.onChanged();
	return false;
};

ms.unorderedAlternatives.prototype.save = function() {
	this.savedList = Object.assign({}, this.list);
};

ms.unorderedAlternatives.prototype.restore = function() {
	this.list = Object.assign({}, this.savedList);
};

ms.valueProperty = function(name, opt_cost_term) {
	this.name = name;
	this.value = null;
	this.savedValue = null;
	this.parent = null;
	this.cost_term = opt_cost_term || '';
	if (this.cost_term && !ms.nodeStats.costTerms.includes(this.cost_term)) {
		alert('Invalid cost term.');
	}
};

ms.valueProperty.prototype.getName = function() {
	return this.name;
};

ms.valueProperty.prototype.setParent = function(parent) {
	this.parent = parent;
};

ms.valueProperty.prototype.forEachNeighbor = function(func) {};

ms.valueProperty.prototype.get = function() {
	return this.value;
};

ms.valueProperty.prototype.set = function(value) {	
	var delta = value - (this.value || 0);
	if (this.cost_term) {
		this.applyCostDelta(delta);
	}
	this.value = value;
};

ms.valueProperty.prototype.applyCostDelta = function(delta) {
	if (delta == 0) {
		return;
	}
	var costChange = this.parent.stats.costChange;
	costChange[this.cost_term] += delta;
	if (ms.optimizer.detailedCost) {
		if (!costChange.details.hasOwnProperty(this.parent.id)) {
			costChange.details[this.parent.id] = 0;
		}
		costChange.details[this.parent.id] += delta;
	}
};

ms.valueProperty.prototype.destroy = function() {
	if (this.cost_term) {
		this.applyCostDelta(-this.value);
	}
};

ms.valueProperty.prototype.save = function() {
	this.savedValue = this.value;
};

ms.valueProperty.prototype.restore = function() {
	this.value = this.savedValue;
};
