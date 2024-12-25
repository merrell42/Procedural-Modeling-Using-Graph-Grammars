ms.shapeGroup = function() {
	this.shapes = [];
};

ms.shapeGroup.prototype.addShape = function(shape) {
	this.shapes.push(shape);
};

ms.shapeGroup.prototype.getVerticesFromIndex = function(index) {
	return this.shapes.map(function(shape) {
		return shape.getVertexFromIndex(index);
	});
};

ms.shapeGroup.prototype.getVertexGroupsFromIndex = function(index) {
	return this.shapes.map(function(shape) {
		return shape.getVertexFromIndex(index).getGroup();
	});
};

ms.shapeGroup.prototype.apply = function(func) {
	this.shapes.forEach(function(shape) {
		func(shape);
	});
};