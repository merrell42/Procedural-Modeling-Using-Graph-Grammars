ms.decorationModel = function(exampleShape) {
	this.brushCollections = [new ms.brushCollection(), new ms.brushCollection()];
	this.brushCollections[0].createDefault(this.notify.bind(this));
	this.brushCollections[1].createArea(this.notify.bind(this));
	this.collectionIndex = ms.decorationModel.Mode.BRUSH;
	this.brushIndex = 0;
	this.vertex = null;
	this.exampleShape = exampleShape;
	this.showDraggables = false;
	this.observers = [];
	this.draggables = [];
};

ms.decorationModel.Mode = {
	BRUSH: 0,
	AREA: 1,
	VERTEX: 2,
};

ms.decorationModel.prototype.addObserver = function(observer) {
	this.observers.push(observer);
};

ms.decorationModel.prototype.getMode = function() {
	if (this.vertex) {
		return ms.decorationModel.Mode.VERTEX;
	}
	return this.collectionIndex;
};

ms.decorationModel.prototype.setBrushIndex = function(brushIndex) {
	this.brushIndex = brushIndex;
	this.updateDraggableSet();
};

ms.decorationModel.prototype.setActiveBrush = function(brush) {
	this.brushIndex = this.brushCollections[this.getMode()].getBrushes().indexOf(brush);
	this.updateDraggableSet();
	this.notify();
};

ms.decorationModel.prototype.getActiveBrush = function() {
	// We can be in vertex mode in objectController.selectMode.
	var brushMode = this.getMode() % 2;
	return this.brushCollections[brushMode].getBrushes()[this.brushIndex];
};

ms.decorationModel.prototype.getActiveObject = function() {
	if (this.getMode() == ms.decorationModel.Mode.VERTEX) {
		return this.vertex.getDecoration();
	} else {
		return this.getActiveBrush();
	}
};

ms.decorationModel.prototype.getActiveCollection = function() {
	if (this.getMode() == ms.decorationModel.Mode.VERTEX) {
		return null;
	}
	return this.brushCollections[this.getMode()];
};

ms.decorationModel.prototype.getVertex = function() {
	return this.vertex;
};

ms.decorationModel.prototype.setVertex = function(vertex) {
	this.vertex = vertex;	
	if (vertex) {
		var decoration = vertex.getDecoration();
		if (!decoration) {
			decoration = new ms.vertexDecoration(this.notify.bind(this));
			vertex.setDecoration(decoration);
		}
	}
	this.updateDraggableSet();
	this.notify();
};

ms.decorationModel.prototype.setMode = function(controllerMode) {
	var cModeTypes = ms.objectController.modeTypes;
	var dModeTypes = ms.decorationModel.Mode;
	this.showDraggables = false;
	if (controllerMode == cModeTypes.DECORATION) {
		this.showDraggables = true;
	} else if (controllerMode == cModeTypes.FILL) {
		this.collectionIndex = dModeTypes.AREA;
	} else {
		this.collectionIndex = dModeTypes.BRUSH;
	}
	this.updateDraggableSet();
	this.notify();
};

ms.decorationModel.prototype.setExampleShape = function(exampleShape) {
	this.exampleShape = exampleShape;
};

ms.decorationModel.prototype.notify = function() {
	this.observers.forEach(function(observer) {
		observer.notify();
	});
};

ms.decorationModel.prototype.getBrushDraggables = function() {
	var draggables = [];
	var activeBrush = this.getActiveBrush();
	var spacing = ms.globalSettings.get('Major Grid Spacing');
	var edges = this.exampleShape.getEdges();
	edges = edges.filter(function(edge) { return edge.brush == activeBrush; });
	edges.forEach(function(edge) {
		var startPos = edge.getStart().getPosition();
		var endPos = edge.getEnd().getPosition();
		var u = endPos.copy().minus(startPos).normalize();
		var v = new ms.vec2(-u.y, u.x);

		if (activeBrush.hasTile()) {
			// Tile Length
			{
				var posFromBrush = function() {
					var tileLength = spacing * activeBrush.get('Tile Length');
					return startPos.copy().add(u.copy().scale(tileLength));
				};
				var brushFromPos = function(pos) {
					var newTileLength = u.dot(pos.copy().minus(startPos.copy())) / spacing;
					activeBrush.set('Tile Length', newTileLength);
				};
				draggables.push(new ms.draggableLine(u, posFromBrush, brushFromPos));
			}

			// Width 1
			{
				var posFromBrush = function() {
					var offset = spacing * activeBrush.get('Offset');
					var width = spacing * activeBrush.get('Width');
					return startPos.copy().add(v.copy().scale(width / 2 - offset));
				};
				var brushFromPos = function(pos) {
					var offset = spacing * activeBrush.get('Offset');
					var newWidth = 2 * (offset + v.dot(pos.copy().minus(startPos.copy()))) / spacing;
					activeBrush.set('Width', newWidth);
				};
				draggables.push(new ms.draggableLine(v, posFromBrush, brushFromPos));
			}

			// Width 2
			{
				var posFromBrush = function() {
					var offset = spacing * activeBrush.get('Offset');
					var width = spacing * activeBrush.get('Width');
					return startPos.copy().add(v.copy().scale(-width / 2 - offset));
				};
				var brushFromPos = function(pos) {
					var offset = spacing * activeBrush.get('Offset');
					var newWidth = 2 * (-offset - v.dot(pos.copy().minus(startPos.copy()))) / spacing;
					activeBrush.set('Width', newWidth);
				};
				draggables.push(new ms.draggableLine(v, posFromBrush, brushFromPos));
			}

			// Offset
			{
				var posFromBrush = function() {
					var offset = spacing * activeBrush.get('Offset');
					return startPos.copy().add(v.copy().scale(-offset));
				};
				var brushFromPos = function(pos) {
					var newOffset = -v.dot(pos.copy().minus(startPos.copy())) / spacing;
					activeBrush.set('Offset', newOffset);
				};
				draggables.push(new ms.draggableLine(v, posFromBrush, brushFromPos));
			}
		}

	});
	return draggables;
};

ms.decorationModel.prototype.getVertexDraggables = function() {
	var decoration = this.vertex.getDecoration();
	var draggables = [];
	var spacing = ms.globalSettings.get('Major Grid Spacing');

	var startPos = this.vertex.getPosition();
	var u = new ms.vec2(1, 0);
	var v = new ms.vec2(0, 1);
	
	var startPos = this.vertex.getPosition();

	if (decoration.hasImage()) {
		// Center
		var anchorFromBrush = function() {
			var anchorX = spacing * decoration.get('Anchor X');
			var anchorY = spacing * decoration.get('Anchor Y');
			return startPos.copy().add(new ms.vec2(anchorX, anchorY));
		};
		var centerFromBrush = function() {
			var centerX = spacing * decoration.get('Center X');
			var centerY = spacing * decoration.get('Center Y');
			return startPos.copy().add(new ms.vec2(centerX, centerY));
		};
		{
			var brushFromPos = function(pos) {
				pos.minus(startPos);
				decoration.set('Center X', pos.x / spacing);
				decoration.set('Center Y', pos.y / spacing);
			};
			var brushFromAnchor = function(pos) {
				pos.minus(startPos);
				decoration.set('Anchor X', pos.x / spacing);
				decoration.set('Anchor Y', pos.y / spacing);
			};
			draggables.push(new ms.draggablePoint(centerFromBrush, brushFromPos, anchorFromBrush, brushFromAnchor));
		}

		// Width 1
		{
			var posFromBrush = function() {
				var centerX = spacing * decoration.get('Center X');
				var centerY = spacing * decoration.get('Center Y');
				var width = spacing * decoration.get('Width');
				var result = startPos.copy().add(u.copy().scale(width / 2));
				result.move(centerX, centerY);
				return result;
			};
			var brushFromPos = function(pos) {
				var w = spacing * decoration.get('Width');
				var anchor = anchorFromBrush();
				var center = centerFromBrush();
				var a    = anchor.x;
				var p    = center.x + w / 2;
				var pNew = pos.x;
				var b    = p - w;    // b is the opposite boundary from p.
				var bNew = a + (b - a) * (pNew - a) / (p - a);
				var wNew = pNew - bNew;
				var cNewX = (pNew + bNew) / 2;				
				if (decoration.get('Fix Aspect Ratio')) {
					var h = spacing * decoration.get('Height');
					var newH = wNew * decoration.get('Height') / decoration.get('Width');					
					var c = center.y;
					var a = anchor.y;
					var cNewY = (c * newH - a * (newH - h)) / h;					
					decoration.set('Height', newH / spacing);
					decoration.set('Center Y', (cNewY - startPos.y) / spacing);
				}
				decoration.set('Width', wNew / spacing);
				decoration.set('Center X', (cNewX - startPos.x) / spacing);
			};
			draggables.push(new ms.draggableLine(u, posFromBrush, brushFromPos));
		}

		// Width 2
		{
			var posFromBrush = function() {
				var centerX = spacing * decoration.get('Center X');
				var centerY = spacing * decoration.get('Center Y');
				var width = spacing * decoration.get('Width');
				var result = startPos.copy().minus(u.copy().scale(width / 2));
				result.move(centerX, centerY);
				return result;
			};
			var brushFromPos = function(pos) {
				var w = spacing * decoration.get('Width');
				var anchor = anchorFromBrush();
				var center = centerFromBrush();
				var a    = anchor.x;
				var p    = center.x - w / 2;
				var pNew = pos.x;
				var b    = p + w;    // b is the opposite boundary from p.
				var bNew = a + (b - a) * (pNew - a) / (p - a);
				var wNew = bNew - pNew;
				var cNewX = (pNew + bNew) / 2;				
				if (decoration.get('Fix Aspect Ratio')) {
					var h = spacing * decoration.get('Height');
					var newH = wNew * decoration.get('Height') / decoration.get('Width');					
					var c = center.y;
					var a = anchor.y;
					var cNewY = (c * newH - a * (newH - h)) / h;					
					decoration.set('Height', newH / spacing);
					decoration.set('Center Y', (cNewY - startPos.y) / spacing);
				}
				decoration.set('Width', wNew / spacing);
				decoration.set('Center X', (cNewX - startPos.x) / spacing);
			};
			draggables.push(new ms.draggableLine(u, posFromBrush, brushFromPos));
		}

		// Height 1
		{
			var posFromBrush = function() {
				var centerX = spacing * decoration.get('Center X');
				var centerY = spacing * decoration.get('Center Y');
				var height = spacing * decoration.get('Height');
				var result = startPos.copy().add(v.copy().scale(height / 2));
				result.move(centerX, centerY);
				return result;
				
			};
			var brushFromPos = function(pos) {
				var h = spacing * decoration.get('Height');
				var anchor = anchorFromBrush();
				var center = centerFromBrush();
				var a    = anchor.y;
				var p    = center.y + h / 2;
				var pNew = pos.y;
				var b    = p - h;    // b is the opposite boundary from p.
				var bNew = a + (b - a) * (pNew - a) / (p - a);
				var hNew = pNew - bNew;
				var cNewY = (pNew + bNew) / 2;				
				if (decoration.get('Fix Aspect Ratio')) {
					var w = spacing * decoration.get('Width');
					var newW = hNew * decoration.get('Width') / decoration.get('Height');
					var c = center.x;
					var a = anchor.x;
					var cNewX = (c * newW - a * (newW - w)) / w;
					decoration.set('Width', newW / spacing);
					decoration.set('Center X', (cNewX - startPos.x) / spacing);
				}
				decoration.set('Height', hNew / spacing);
				decoration.set('Center Y', (cNewY - startPos.y) / spacing);
			};
			draggables.push(new ms.draggableLine(v, posFromBrush, brushFromPos));
		}

		// Height 2
		{
			var posFromBrush = function() {
				var centerX = spacing * decoration.get('Center X');
				var centerY = spacing * decoration.get('Center Y');
				var height = spacing * decoration.get('Height');
				var result = startPos.copy().add(v.copy().scale(-height / 2));
				result.move(centerX, centerY);
				return result;
			};
			var brushFromPos = function(pos) {
				var h = spacing * decoration.get('Height');
				var anchor = anchorFromBrush();
				var center = centerFromBrush();
				var a    = anchor.y;
				var p    = center.y - h / 2;
				var pNew = pos.y;
				var b    = p + h;    // b is the opposite boundary from p.
				var bNew = a + (b - a) * (pNew - a) / (p - a);
				var hNew = bNew - pNew;
				var cNewY = (pNew + bNew) / 2;				
				if (decoration.get('Fix Aspect Ratio')) {
					var w = spacing * decoration.get('Width');
					var newW = hNew * decoration.get('Width') / decoration.get('Height');
					var c = center.x;
					var a = anchor.x;
					var cNewX = (c * newW - a * (newW - w)) / w;
					decoration.set('Width', newW / spacing);
					decoration.set('Center X', (cNewX - startPos.x) / spacing);
				}
				decoration.set('Height', hNew / spacing);
				decoration.set('Center Y', (cNewY - startPos.y) / spacing);
			};
			draggables.push(new ms.draggableLine(v, posFromBrush, brushFromPos));
		}
	}

	return draggables;
};

ms.decorationModel.prototype.updateDraggableSet = function() {
	var ModeTypes = ms.decorationModel.Mode;
	if (this.showDraggables) {
		switch(this.getMode()) {
			case ModeTypes.BRUSH:
				this.draggables = this.getBrushDraggables();
				break;
			case ModeTypes.VERTEX:
			case ModeTypes.AREA:
				this.draggables = this.getVertexDraggables();
				break;
		}
	} else {
		this.draggables = [];
	}
};

ms.decorationModel.prototype.getRenderables = function() {
	return this.draggables;
};

ms.decorationModel.prototype.selectNone = function() {
	this.draggables.forEach(function(draggable) {
		draggable.deselect();
	});
};

ms.decorationModel.prototype.import = function(version, data) {
	if (version == 2) {
		this.brushCollections[0].import(version, data, false, this.notify.bind(this));
	}
	if (version >= 3) {
		this.brushCollections[0].import(version, data, false, this.notify.bind(this));
		this.brushCollections[1].import(version, data, true, this.notify.bind(this));
	}
	this.brushIndex = 0;
}