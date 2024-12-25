// The controller for examples. Handles keyboard and mouse events.
ms.objectController = function(view, shapeMaker, decorationModel, mainController) {
	this.view = view;
	this.mainController = mainController;
	this.shapeMaker = shapeMaker;

	var edgeSelector = new ms.edgeSelector(shapeMaker);
	var shapeView = view.getSubView(ms.view.modes.SHAPE);
	this.selector = new ms.selector(shapeMaker, edgeSelector);
	this.selector.register(shapeView);
	this.decorationModel = decorationModel;
	this.cameraController = new ms.cameraController(view, this.notify.bind(this));

	this.subMode = ms.objectController.subModeTypes.SELECT;
	var w = ms.mainController.getCanvasWidth();
	var h = ms.mainController.getCanvasHeight();
	this.viewport = new ms.viewport(1, w / 2, h / 2);
	this.mover = new ms.mover(shapeMaker.exampleShape);
	this.isShifted = false;
};

ms.objectController.modeTypes = {
	SELECT_RECT: 0,
	CREATE_POLYGON: 1,
	LASSO: 3,
	FILL: 4,
	DECORATION: 5,
};

ms.objectController.prototype.isRigid = function() {
	return this.mode == ms.objectController.modeTypes.CREATE_RIGID;
};

ms.objectController.actionButtons = {
	NEW_SHAPE: 0,
	SPLIT: 1,
	CLEAR: 2,
	EXPORT: 3,
	BOUNDARY: 4,
	GRAPH_TEMPLATE: 5,
};

ms.objectController.prototype.updateBrush = function() {
	var selectedShape = this.shapeMaker.selectedShape();
	var edges = selectedShape ? selectedShape.getShape().getEdges() : [];
	var brush = edges.length > 0 ? edges[0].getBrush() : null;
	if (brush) {
		this.decorationModel.setVertex(null);
		this.decorationModel.setActiveBrush(brush);
	}
};

ms.objectController.prototype.activate = function(opt_mode) {
	var modeButtons = ms.mvp ? [] : ['Select Rect', 'Polygon', 'Lasso', 'Fill', 'Decoration'];
	var actionButtons = ms.mvp ? [] : ['New', 'Split', 'Clear', 'Export', 'Boundary', 'Graph Template'];
	this.mainController.setModeButtons(modeButtons);
	this.mainController.setActionButtons(actionButtons);
	var is3D = this.shapeMaker.getRenderables()[0].shapes[0].is3D;
	var mode = is3D ? ms.view.modes.VIEW3D : ms.view.modes.SHAPE;
	this.view.setMode(mode, ms.mainController.controllerTypes.OBJECT);
	this.viewport = this.view.getViewport();
	// this.viewport.setBaseScale(1);
	this.updateBrush();
	
	var mode = opt_mode || ms.objectController.modeTypes.SELECT_RECT;
	this.mainController.updateMode(mode);
	ms.guideMutator.hideStats();
};

ms.objectController.subModeTypes = {
	NONE: 0,
	SELECT: 1,
	CONTROL_POINT: 2,
	MOVE: 3
};

ms.objectController.prototype.notify = function() {
	var is3D = this.shapeMaker.getRenderables()[0].shapes[0].is3D;
	var expectedMode = is3D ? ms.view.modes.VIEW3D : ms.view.modes.SHAPE;
	if (this.view.mode != expectedMode) {
		this.view.setMode(expectedMode);
		this.viewport = this.view.getViewport();
		this.cameraController.activate();
	}
	return this.shapeMaker.notify();
};

ms.objectController.prototype.updateMode = function(mode) {
	this.mode = mode;
	var modeTypes = ms.objectController.modeTypes;
	if ((this.mode != modeTypes.LASSO) && (this.mode != modeTypes.SELECT_RECT)) {
		this.selector.selectNone();
	}
	if (this.mode == modeTypes.CREATE_POLYGON) {
		this.subMode = ms.objectController.subModeTypes.NONE;
		this.notify();
	}
	this.decorationModel.setMode(this.mode);
	var exampleShape = this.shapeMaker.exampleShape;
	if (!exampleShape.selectedShape) {
		exampleShape.selectedShape = exampleShape.shapes[0];
		this.notify();
	}
	
};

ms.objectController.prototype.applyAction = function(button) {
	var actionButtons = ms.objectController.actionButtons;
	if (button == actionButtons.NEW_SHAPE) {
		this.shapeMaker.newShape();
	} else if (button == actionButtons.SPLIT) {
		this.shapeMaker.split();
		this.mainController.updateController(ms.mainController.controllerTypes.EXAMPLE);
	} else if (button == actionButtons.CLEAR) {
		this.shapeMaker.clear();
		this.mainController.updateMode(ms.objectController.modeTypes.CREATE_POLYGON);
		this.notify();
	} else if (button == actionButtons.EXPORT) {
		navigator.clipboard.writeText(ms.shapeMaker.export() + ',\'' + ms.globalSettings.export() + '\'');
	} else if (button == actionButtons.BOUNDARY) {
		this.addBoundary();
	} else if (button == actionButtons.BOUNDARY) {
		this.addBoundary();
	} else if (button == actionButtons.GRAPH_TEMPLATE) {
		navigator.clipboard.writeText(ms.shapeMaker.exportGraphTemplate());
	}
};

ms.objectController.prototype.onmousemove = function (event) {	
	if (this.view.mode == ms.view.modes.VIEW3D) {
		this.cameraController.onmousemove(event);
		return;
	}

	var ModeTypes = ms.objectController.modeTypes;
	var SubModeTypes = ms.objectController.subModeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	var scale = this.view.getViewport().scale;

	switch(this.subMode) {
		case SubModeTypes.MOVE:
		case SubModeTypes.CONTROL_POINT:
			var dx = position.x - this.prevMouse.x;
			var dy = position.y - this.prevMouse.y;
			if (this.subMode == SubModeTypes.MOVE) {
				this.mover.move(dx, dy, event.ctrlKey);
				this.notify();
			} else {
				this.shapeMaker.controlPointMove(dx, dy);
			}
			this.prevMouse = position;
			break;
		case SubModeTypes.SELECT:
			switch(this.mode) {
				case ModeTypes.SELECT_RECT:   this.selector.moveSelectRect(position);  break;
				case ModeTypes.LASSO:         this.selector.addLasso(position);        break;
			}
			break;
		case SubModeTypes.NONE:
			switch(this.mode) {
				case ModeTypes.CREATE_POLYGON:
					if (this.shapeMaker.newVertex == null) {
						var newVertex = new ms.exampleVertex(position.x, position.y);
						this.shapeMaker.addVertex(newVertex);
						newVertex.getGroup().select();
						
						// TODO: Don't use private variable. Maybe put mover into shapeMaker.
						this.mover.startMove(newVertex.getGroup());
					} else {
						var dx = position.x - this.prevMouse.x;
						var dy = position.y - this.prevMouse.y;
						this.mover.move(dx, dy);
						this.shapeMaker.updateNewVertex();
					}
					this.prevMouse = position;
					break;
				case ModeTypes.SELECT_RECT:
				case ModeTypes.LASSO:
				case ModeTypes.DECORATION:
					var v = this.selector.findVertex(position, scale);
					this.shapeMaker.hover(v);
					break;
			}
			break;
	}
};

ms.objectController.prototype.onmouseup = function (event) {
	if (this.view.mode == ms.view.modes.VIEW3D) {
		this.cameraController.onmouseup(event);
		return;
	}

	if (event.button == 2) {
		return;
	}
	var ModeTypes = ms.objectController.modeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	if ((this.mode == ModeTypes.CREATE_POLYGON) && event.which != 3) {
		this.shapeMaker.placeVertex();
	} else if (this.mode == ModeTypes.FILL) {
		this.shapeMaker.fillArea(position, this.decorationModel.getActiveBrush());
	} else {
		if (this.subMode == ms.objectController.subModeTypes.SELECT) {
			if (this.mode == ModeTypes.LASSO) {
				this.selector.closeLasso();
			}
			this.selector.select(event.ctrlKey, event.altKey);
		} else if (this.subMode == ms.objectController.subModeTypes.MOVE) {
			this.mover.finishMove();
			this.mover.startMove(this.shapeMaker.selectedShape().getShape());
		}
		this.subMode = ms.objectController.subModeTypes.NONE;
	}
};

ms.objectController.prototype.onmousedown = function (event) {	
	if (this.view.mode == ms.view.modes.VIEW3D) {
		this.cameraController.onmousedown(event);
		return;
	}

	var modeTypes = ms.objectController.modeTypes;
	var selectTypes = ms.shapeMaker.SelectableTypes;
	var submodeTypes = ms.objectController.subModeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	var scale = this.view.getViewport().scale;
	if ((this.mode == modeTypes.SELECT_RECT) || (this.mode == modeTypes.LASSO) ||
	    (this.mode == modeTypes.DECORATION)) {
		var selected = null;
		var options = {
			add: event.ctrlKey,
			subtract: event.altKey,
			scale: scale,
			types: [selectTypes.VERTEX, selectTypes.EDGE, selectTypes.DRAGGABLE],
		};
		selected = this.selector.pointSelect(position, options);
		if (selected) {
			this.subMode = submodeTypes.MOVE;
		} else {
			this.subMode = submodeTypes.SELECT;
		}		
		if (this.mode == modeTypes.DECORATION) {
			if (!selected) {
				this.decorationModel.setVertex(null);
			} else if (selected.selectType() == selectTypes.VERTEX) {
				this.decorationModel.setVertex(selected);
			}
		}		
		this.prevMouse = position;
		if (selected) {
			if (selected.selectType() == selectTypes.VERTEX) {
				this.mover.startMove(this.shapeMaker.selectedShape().getShape());
			} else {
				this.mover.startMove(selected);
			}
		} else {
			switch(this.mode) {
				case modeTypes.SELECT_RECT:   this.selector.createSelectRect(this.prevMouse);   break;
				case modeTypes.LASSO:         this.selector.createSelectLasso(this.prevMouse);  break;
			}
		}
	}
};

ms.objectController.prototype.onmouseout = function (event) {
	var modeTypes = ms.objectController.modeTypes;
	if (this.mode == modeTypes.CREATE_POLYGON) {
		this.shapeMaker.removeVertex();
	}
};

ms.objectController.prototype.onDoubleClick = function (event) {
	var modeTypes = ms.objectController.modeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	var scale = this.view.getViewport().scale;
	var selectTypes = ms.shapeMaker.SelectableTypes;

	var options = {
		scale: scale,
		types: [selectTypes.VERTEX, selectTypes.EDGE],
	};
	var selected = this.selector.pointSelect(position, options);
	// if (!selected) { this.mainController.updateController(ms.mainController.controllerTypes.EXAMPLE); }
	if (selected) {
		if (selected.selectType() == selectTypes.VERTEX) {
			this.decorationModel.setVertex(selected);
		} else {
			this.decorationModel.setVertex(null);
			this.decorationModel.setActiveBrush(selected.getBrush());
		}
		this.mainController.updateMode(modeTypes.DECORATION);
	}
};

ms.objectController.prototype.onRightClick = function (event) {
	var modeTypes = ms.objectController.modeTypes;
	if (this.mode == modeTypes.CREATE_POLYGON && this.shapeMaker.getNewEdge()) {
		this.shapeMaker.removeVertex();
		this.shapeMaker.clearPrevVertex();
		return;
	} else if (this.mode == modeTypes.SELECT_RECT || this.mode == modeTypes.FILL) {
		var scale = this.view.getViewport().scale;
		var selectTypes = ms.shapeMaker.SelectableTypes;
		var options = {
			scale: scale,
			types: [selectTypes.EDGE, selectTypes.FACE],
		};
		var screenPosition = new ms.vec2(event.x, event.y);
		var position = this.view.getViewport().inverseTransform(screenPosition);
		var selected = this.selector.pointSelect(position, options);
		if (selected && selected.selectType() == selectTypes.EDGE) {
			this.mainController.updateMode(modeTypes.CREATE_POLYGON);
			this.decorationModel.setActiveBrush(selected.getBrush());
			return;
		}
		if (selected && selected.selectType() == selectTypes.FACE &&
			selected.getArea().getColor() != '#fff') {
			this.mainController.updateMode(modeTypes.FILL);
			this.decorationModel.setActiveBrush(selected.getArea());
			return;
		}
	}
	return this.onEscape(event);	
};

ms.objectController.prototype.onEscape = function (event) {
	var modeTypes = ms.objectController.modeTypes;
	this.shapeMaker.removeVertex();
	this.shapeMaker.clearPrevVertex();
	this.mainController.updateMode(modeTypes.SELECT_RECT);
};

ms.objectController.prototype.onKeyPress = function (event) {
	var modeTypes = ms.objectController.modeTypes;
	var keycode = event.keyCode;
	switch (keycode) {
		// Shift
		case 16:
			if (this.shapeMaker.newVertex) {
				if (this.shapeMaker.prevVertex) {
					var movement = this.shapeMaker.newVertex.getPosition().copy();
					movement.minus(this.shapeMaker.prevVertex.getPosition());
					this.mover.align(this.shapeMaker.newVertex.getGroup(), movement);
				} else {
					this.mover.align(this.shapeMaker.newVertex.getGroup());
				}
			} else {
				this.mover.align(this.shapeMaker.selectedShape().getShape());
			}
			this.notify();
			break;
		// Escape
		case 27:	this.onEscape();	break;
		// Delete
		case 46:	this.shapeMaker.deleteSelected();	break;
		// Ctrl-A
		case 65:
			if (event.ctrlKey) {
				this.selector.selectAll();
			}                         
			// event.disableTextSelect();
			break;
		// Fill Area (F)
		case 70:
			// F			
			this.shapeMaker.removeVertex();
			this.shapeMaker.clearPrevVertex();
			this.mainController.updateMode(modeTypes.FILL); break;
			break
		// Polygon
		case 80:
			//P
			this.mainController.updateMode(modeTypes.CREATE_POLYGON); break;
			break;
		default:	break;
	}
};

ms.objectController.prototype.onKeyUp = function (event) {
	var keycode = event.keyCode;
	switch (keycode) {
		case 16:
			this.mover.unalign();
			this.notify();
			break;
		default:	break;
	}
};

ms.objectController.prototype.getExample = function() {
	return this.shapeMaker.getShape();
};

ms.objectController.prototype.addBoundary = function (event) {
	var onChange = this.decorationModel.notify.bind(this.decorationModel);
	var newBrush = new ms.brush('#000', '#000', onChange);
	newBrush.set('Bend', 0);
	newBrush.set('Boundary', true);
	this.decorationModel.brushCollections[0].addThisBrush(newBrush);
	var brushSelector = this.mainController.brushSelector;
	brushSelector && brushSelector.notify();
	
	var boundaryArea = new ms.area('#bde', '#bde', () => {});
	boundaryArea.set('Boundary', true);
	this.decorationModel.brushCollections[1].addThisBrush(boundaryArea);
	
	this.shapeMaker.addBoundary(newBrush, boundaryArea);	
};
