ms.faceConnection = function(stats, coordinates) {
	var properties = {
			cell: new ms.alternativeArray('cell', /* required */ false),
			// The face which is the right bound of the connection.
			face: new ms.singleProperty('face', /* required */ true),
			// The endpoint that is the left bound is first, the right bound second.
			endpoint: new ms.requiredArray('endpoint', /* required */ true, 2),
			// The position of the endpoints. The left bound is first, the right bound second.
			coordinates: new ms.valueProperty('coordinates'),
	};
	this.node = new ms.node(this, stats, 'faceConnection', properties);
	this.node.setValue('coordinates', coordinates);
};

ms.faceConnection.prototype.getNode = function() {
	return this.node;
};

ms.faceConnection.prototype.getEndpoints = function() {
	return this.node.get('endpoint');
};

ms.faceConnection.prototype.getFace = function() {
	return this.node.get('face');
};

ms.faceConnection.prototype.getCoordinates = function() {
	return this.node.get('coordinates');
};

ms.faceConnection.prototype.getCells = function() {
	return this.node.get('cell');
};

// Returns true if the endpoint is the left bound of the connection.
ms.faceConnection.prototype.isLeft = function(endpoint) {
	return this.getEndpoints()[0] == endpoint;
};

ms.faceConnection.prototype.addCells = function() {
	var self = this;
	var coordinates = this.getCoordinates();
	var x0 = Math.floor(coordinates[0].x);
	var x1 = Math.floor(coordinates[1].x);
	var y  = Math.floor(coordinates[0].y);
	var model = this.node.getModel();
	for (var x = x0; x <= x1; x++) {
		model.getCell(x, y).addFaceConnection(this);
	}
};

ms.faceConnection.prototype.intersectsState = function(lineState) {
	var endpoints = lineState.getLine().getEndpoints();
	var state0 = endpoints[0].getPosition();
	var state1 = endpoints[1].getPosition();
	var coordinates = this.getCoordinates();
	return !!ms.intersector.intersect(coordinates[0], coordinates[1], state0, state1, 0);
};

/* ms.faceConnection.prototype.removeCells = function() {
	var oldCells = this.getCells().slice();
	var self = this;
	oldCells.forEach(function(cell) {
		cell.removeFaceConnection(self);
	});
}; */

ms.faceConnection.prototype.highlight = function(context, convertToScreen) {
	this.draw(context, convertToScreen, true);
};

ms.faceConnection.prototype.draw = function(context, convertToScreen, opt_highlighted) {
	var coordinates = this.getCoordinates();
	var p0 = convertToScreen(coordinates[0]);
	var p1 = convertToScreen(coordinates[1]);
	context.strokeStyle = opt_highlighted ? '#f0f' : '#fcad03';
	context.lineWidth = opt_highlighted ? 2 : 1;
	context.globalAlpha = 1;
	context.beginPath();
	context.moveTo(p0.x, p0.y);
	context.lineTo(p1.x, p1.y);
	context.stroke();
};

ms.faceConnection.prototype.print = function() {
	ms.highlight(this);
};
