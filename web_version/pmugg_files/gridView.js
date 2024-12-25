ms.gridView = function(gridCanvas, objectCanvas, offscreenCanvas) {
	this.gridCanvas = gridCanvas;
	this.context = gridCanvas.getContext('2d');
	this.objectCanvas = objectCanvas;
	this.offscreenCanvas = offscreenCanvas;
	this.viewport = new ms.viewport(1, 1, 1);
};

ms.gridView.MARGIN = 25;
// ms.gridView.ZOOM_FACTOR = 5;

ms.gridView.prototype.getViewport = function() {
	return this.viewport;
};

ms.gridView.prototype.setExtents = function(extents) {
	var margin = ms.gridView.MARGIN;
	var canvasWidth = this.gridCanvas.width;
	var canvasHeight = this.gridCanvas.height;
	// The mutation area has a margin of 1 for historical reasons.
	// Subtract the margin to better fill the screen.
	var scale = Math.min(
	    (canvasWidth - margin) / (extents[0] - 1),
	    (canvasHeight - margin) / (extents[1] - 1));
	this.viewport = new ms.viewport(scale, extents[0] / 2, extents[1] / 2);
};

ms.gridView.prototype.resize = function(width, height) {
	this.gridCanvas.width = width;
	this.gridCanvas.height = height;
	this.offscreenCanvas.width = width;
	this.offscreenCanvas.height = height;
};

ms.gridView.prototype.activate = function() {
	this.gridCanvas.style.display = '';
	var context = this.objectCanvas.getContext('2d');
};

ms.gridView.prototype.deactivate = function() {
	this.gridCanvas.style.display = 'none';
};

ms.highlightedElement = null;

ms.highlight = function(element) {
	ms.highlightedElement = element;
	if (!ms.globalMutator) {
		ms.alert('Global Mutator not yet defined.');
	} else {
		ms.globalMutator.notify(true);
	}
};

ms.gridView.prototype.redraw = function(model, fullRedraw) {
	if (!(model instanceof ms.model)) {
		// This can happen when shapeMaker redraws while in synthesis mode.
		// TODO: Clean this up.
		return;
	}
	var extents = model.getExtents();
	var context = this.context;
	context.imageSmoothingQuality = ms.globalSettings.get('High Smoothing') ? 'high' : 'low';
	context.clearRect(0, 0, this.gridCanvas.width, this.gridCanvas.height);

	var self = this;
	var convertToScreenX = function(x) {
		return self.viewport.transformX(x);
	}
	var convertToScreenY = function(y) {
		return self.viewport.transformY(extents[1] - y);
	}	
	var convertToScreen = function(position) {
		return {
			x: convertToScreenX(position.x),
			y: convertToScreenY(position.y)
		};
	}
	convertToScreen.scale = self.viewport.scale;
	var drawSquare = function(x, y, color) {
		context.beginPath();
		var x0 = convertToScreenX(x);
		var x1 = convertToScreenX(x + 1);
		var y0 = convertToScreenY(y);
		var y1 = convertToScreenY(y + 1);
		context.rect(x0, y1, x1 - x0, y0 - y1);
		context.fillStyle = 'rgba(' + color + ', 1)';
		context.fill();
	};
	if (fullRedraw) {
		ms.gridView.drawFaces(model, context, convertToScreen);
	}
	if (ms.globalSettings.get('Show Grid')) {
		var faceConnections = model.getElements('faceConnection');
		faceConnections.forEach(function(faceConnection) {
			faceConnection.draw(context, convertToScreen);
		});
	}

	context.lineWidth = 3; // This normally gets overwritten.
	context.strokeStyle = '#080';
	context.globalAlpha = 1;
	for (var x = 0; x < extents[0]; x++) {
		for (var y = 0; y < extents[1]; y++) {
			var cell = model.getCell(x, y, 0);
			var states = cell.getActiveStates();
			for (var i = 0; i < states.length; i++) {
				var stateValue = states[i];
				// This ensures what we do not draw the same state twice.
				if (cell != stateValue.getCells()[0]) {
					continue;
				}
				stateValue.draw(context, convertToScreen);
			}
		}
	}
	context.lineWidth = 1;
	model.getVertices().forEach(function(vertex) {
		vertex.draw(context, convertToScreen);
	});
	// TODO: Remove hack
	// HACK: Shape view stuff is hacky.
	if (ms.highlightedElement && (!ms.highlightedElement.requiresShapeView || !ms.highlightedElement.requiresShapeView())) {
		ms.highlightedElement.highlight(context, convertToScreen);
	}
	
	if (ms.globalSettings.get('Show Grid')) {
		// Draw text.
		context.fillStyle = '#888';
		context.font = '12px Arial';
		context.textAlign = 'center';
		var textPositionY = Math.max(10, convertToScreenY(extents[1] + 0.15));
		var textOffsetX = (textPositionY == 10) ? 8 : 0;
		for (var x = 0; x < extents[0] + 1; x++) {
			context.fillText(x, convertToScreenX(x) + textOffsetX, textPositionY);
		}
		var textPositionX = Math.max(10, convertToScreenX(-0.3));
		if (textPositionX == 10) {
			for (var y = 0; y < extents[1] + 1; y++) {
				context.fillText(y, textPositionX, convertToScreenY(y) - 8);
			}
		} else {
			for (var y = 0; y < extents[1] + 1; y++) {
				context.fillText(y, textPositionX, convertToScreenY(y - 0.1));
			}
		}

		context.lineWidth = 1;
		context.strokeStyle = '#888';
		for (var x = 1; x < extents[0]; x++) {
			context.beginPath();
			context.moveTo(convertToScreenX(x), convertToScreenY(0));
			context.lineTo(convertToScreenX(x), convertToScreenY(extents[1]));
			context.stroke();
		}
		for (var y = 1; y < extents[1]; y++) {
			context.beginPath();
			context.moveTo(convertToScreenX(0),          convertToScreenY(y));
			context.lineTo(convertToScreenX(extents[0]), convertToScreenY(y));
			context.stroke();
		}
	}
};

ms.gridView.drawFaces = function(model, context, convertToScreen) {
	var drawnFaces = [];
	model.getFaces().forEach(function(face) {
		ms.gridView.drawFace(face, drawnFaces, context, convertToScreen);
	});
};

// Draw the enclosed face first.
ms.gridView.drawFace = function(face, drawnFaces, context, convertToScreen) {
	if (drawnFaces.includes(face)) {
		return;
	}
	face.getEndpoints().forEach(function(endpoint) {
		endpoint.getConnections().forEach(function(connection) {
			var endpointB = connection.getEndpoints()[0];
			var faceB = endpointB && endpointB.getFace();
			if (faceB && (faceB != face) && !drawnFaces.includes(faceB)) {
				ms.gridView.drawFace(faceB, drawnFaces, context, convertToScreen);
			}
		});
	});
	
	/* var enclosed = face.enclosedFace();
	if (enclosed && !drawnFaces.includes(enclosed)) {
		ms.gridView.drawFace(enclosed, drawnFaces, context, convertToScreen);
	} */
	face.draw(context, convertToScreen);
	drawnFaces.push(face);
};
