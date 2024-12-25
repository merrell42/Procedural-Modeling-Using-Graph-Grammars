ms.transformedShape = function(shape, opt_offset) {
	this.shape = shape;
	this.selected = false;
	this.is3D = false;
};

ms.transformedShape.counter = 0;

ms.transformedShape.prototype.export = function() {
	var result = []
	// This is where the offset used to be added. It is included just for backwards compatibility
	result.push(0);
	result.push(0);
	result.push(false); // This is obsolete
	return result;
};

ms.transformedShape.import = function(shape, data) {
	var shapeData = data.splice(0, 3);
	var scale = ms.globalSettings.get('Import Scale');
	var offset = new ms.vec2(scale * shapeData[0], scale * shapeData[1]);
	shape.moveSet(offset.x, offset.y);
	// var isRigid = shapeData[2];
	return new ms.transformedShape(shape, null);
};

ms.transformedShape.nullShape = function() {
	return new ms.transformedShape(new ms.shapeSet());
};

ms.transformedShape.prototype.print = function() {
	this.shape.print();
};

ms.transformedShape.prototype.copy = function() {
	return new ms.transformedShape(this.shape.copy(), null);
};

ms.transformedShape.prototype.select = function() {
	this.selected = true;
};

ms.transformedShape.prototype.deselect = function() {
	this.selected = false;
};

ms.transformedShape.prototype.getShape = function() {
	return this.shape;
};

ms.transformedShape.prototype.hover = function(v) {
	return this.shape.hover(v);
};

ms.transformedShape.prototype.getSelected = function() {
	return this.selected;
};

ms.transformedShape.prototype.move = function(dx, dy) {
	this.shape.makeUnique();
	this.shape.moveSet(dx, dy);
};

ms.transformedShape.prototype.getVertices = function() {
	return this.shape.getVertices();
};

ms.transformedShape.prototype.getEdges = function() {
	return this.shape.getEdges();
};

ms.transformedShape.prototype.getColor = function() {
	if (this.selected) {
		return ms.edge.SELECTED_COLOR;
	} else {
		return null;
	}
};

ms.transformedShape.prototype.draw = function(view, offset, opt_color, secondPass) {
	// Not sure if offset is needed.
	this.shape.draw(view, offset, opt_color, secondPass);
};

ms.transformedShape.prototype.addVertex = function(v) {
	this.shape.addVertex(v);
};

ms.transformedShape.prototype.addEdge = function(edge) {
	this.shape.addEdge(edge);
};

ms.transformedShape.prototype.snapIn = function(shape) {
	shape.selectAll();
	shape.selectNone();
	this.shape.snapIn(shape);
};

ms.transformedShape.prototype.selectAll = function() {
	this.shape.selectAll();
};

ms.transformedShape.prototype.selectNone = function() {
	this.shape.selectNone();
};

ms.transformedShape.prototype.fill = function(v, area) {
	this.shape.fill(v, area);
};

ms.transformedShape.prototype.deleteSelected = function() {
	this.shape.deleteSelected();
};

ms.transformedShape.prototype.getKey = function() {
	return this.getShape().getKey();
};

ms.transformedShape.prototype.isEmpty = function() {
	return this.getShape().getVertices().length == 0;
};
