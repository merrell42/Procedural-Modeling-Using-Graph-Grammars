// The controller for examples. Handles keyboard and mouse events.
ms.exampleController = function(view, shapeMaker, exampleShape, decorationModel, mainController) {
	this.view = view;
	this.mainController = mainController;
	this.shapeMaker = shapeMaker;
	this.decorationModel = decorationModel;
	this.exampleShape = exampleShape;
	
	var shapeSelector = new ms.shapeSelector(exampleShape);
	var shapeView = view.getSubView(ms.view.modes.SHAPE);
	this.selector = new ms.selector(shapeMaker, shapeSelector);
	this.selector.register(shapeView);

	this.subMode = ms.exampleController.subModeTypes.SELECT;
	var w = ms.mainController.getCanvasWidth();
	var h = ms.mainController.getCanvasHeight();
	this.viewport = null;
	this.mover = new ms.mover(shapeMaker.exampleShape);
	this.cameraController = new ms.cameraController(view, this.notify.bind(this));
};

ms.exampleController.modeTypes = {
	SELECT_RECT: 0,
	LASSO: 1,
	FILL: 2
};

ms.exampleController.actionButtons = {
	MERGE: 0,
	COPY_LINKED: 1,
	COPY_UNLINKED: 2,
};

ms.exampleController.prototype.activate = function() {
	this.mainController.setModeButtons(['Select Rect', 'Lasso', 'Fill']);
	this.mainController.setActionButtons(['Merge', 'Copy', 'Unlinked']);
	this.view.setMode(ms.view.modes.SHAPE, ms.mainController.controllerTypes.EXAMPLE);
	this.viewport = this.view.getViewport();
	this.cameraController.activate();
	this.mainController.updateMode(ms.exampleController.modeTypes.SELECT_RECT);
	this.exampleShape.setSelectedShape(null);
	this.notify();
};

ms.exampleController.subModeTypes = {
	NONE: 0,
	SELECT: 1,
	MOVE: 2
};

ms.exampleController.prototype.notify = function() {
	this.shapeMaker.notify();
};

ms.exampleController.prototype.updateMode = function(mode) {
	this.mode = mode;
	if ((this.mode != ms.exampleController.modeTypes.LASSO) && (this.mode != ms.exampleController.modeTypes.SELECT_RECT)) {
		this.selector.selectNone();
	}
	this.decorationModel.setMode(
		this.mode == ms.exampleController.modeTypes.FILL ?
		ms.decorationModel.Mode.AREA : 
		ms.decorationModel.Mode.BRUSH);
};

ms.exampleController.prototype.applyAction = function(button) {
	var actionButtons = ms.exampleController.actionButtons;
	if (button == actionButtons.MERGE) {
		this.exampleShape.merge();
	} else if (button == actionButtons.COPY_LINKED) {
		this.exampleShape.copySelected(true);
		this.notify();
	} else if (button == actionButtons.COPY_UNLINKED) {
		this.exampleShape.copySelected(false);
		this.notify();
	}
	this.shapeMaker.selectNone();
};

ms.exampleController.prototype.onmousemove = function (event) {	
	var ModeTypes = ms.exampleController.modeTypes;
	var SubModeTypes = ms.exampleController.subModeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	switch(this.subMode) {
		case SubModeTypes.MOVE:
			var dx = position.x - this.prevMouse.x;
			var dy = position.y - this.prevMouse.y;
			this.mover.move(dx, dy);
			this.notify();
			this.prevMouse.x = position.x;
			this.prevMouse.y = position.y;
			break;
		case SubModeTypes.SELECT:
			switch(this.mode) {
				case ModeTypes.SELECT_RECT:   this.selector.moveSelectRect(position);  break;
				case ModeTypes.LASSO:         this.selector.addLasso(position);        break;
			}
			break;
		case SubModeTypes.NONE:
			switch(this.mode) {
				case ModeTypes.SELECT_RECT:
				case ModeTypes.LASSO:
					var v = this.selector.findVertex(position);
					this.shapeMaker.hover(v);
					break;
			}
			break;
	}
};

ms.exampleController.prototype.onmouseup = function (event) {
	var ModeTypes = ms.exampleController.modeTypes;
	if (this.mode == ModeTypes.FILL) {
		this.shapeMaker.fillArea(event, this.decorationModel.getActiveBrush());
	} else {
		this.mover.finishMove();
		this.mover.startMove(this.exampleShape);
		if (this.subMode == ms.exampleController.subModeTypes.SELECT) {
			if (this.mode == ModeTypes.LASSO) {
				this.selector.closeLasso();
			}
			this.selector.select(event.ctrlKey, event.altKey);
		}
		this.subMode = ms.exampleController.subModeTypes.NONE;
	}
};

ms.exampleController.prototype.onmousedown = function(event) {
	var modeTypes = ms.exampleController.modeTypes;
	var submodeTypes = ms.exampleController.subModeTypes;
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	if ((this.mode == modeTypes.SELECT_RECT) || (this.mode == modeTypes.LASSO)) {
		var isSelected = this.selector.pointSelect(position, {add: event.ctrlKey, subtract: event.altKey});
		if (isSelected) {
			this.subMode = submodeTypes.MOVE;
		} else {
			this.subMode = submodeTypes.SELECT;
		}
		this.downPosition = position;
		this.prevMouse = position;
		if (isSelected) {
			this.mover.startMove(this.exampleShape);
		} else {
			switch(this.mode) {
				case modeTypes.SELECT_RECT:   this.selector.createSelectRect(this.prevMouse);   break;
				case modeTypes.LASSO:         this.selector.createSelectLasso(this.prevMouse);  break;
			}
		}
	}
};

ms.exampleController.prototype.onmouseout = function (event) {};

ms.exampleController.prototype.onDoubleClick = function (event) {
	var screenPosition = new ms.vec2(event.x, event.y);
	var position = this.view.getViewport().inverseTransform(screenPosition);
	var selected = this.selector.pointSelect(position, {});
	if (selected) {
		this.exampleShape.setSelectedShape(selected);
		this.exampleShape.selectNone();
		var objectModes = ms.objectController.modeTypes;
		this.mainController.updateController(ms.mainController.controllerTypes.OBJECT);
		this.notify();
	}
};

ms.exampleController.prototype.onRightClick = function (event) {
	return this.onEscape(event);
};

ms.exampleController.prototype.onEscape = function (event) {
	this.mainController.updateMode(ms.exampleController.modeTypes.SELECT_RECT);
	return false;
};

ms.exampleController.prototype.onKeyPress = function (event) {
	var keycode = event.keyCode;
	switch (keycode) {
		// Shift
		case 16:
			this.mover.align(this.exampleShape);
			this.notify();
			break;
		// Escape
		case 27:	this.onEscape();	break;
		// Delete
		case 46:
			this.exampleShape.deleteSelected();
			this.notify();
			break;
		// Ctrl-A
		case 65:
			if (event.ctrlKey) {
				this.selector.selectAll();
			}                         
			// event.disableTextSelect();
			break;
		default:	break;
	}
};

ms.exampleController.prototype.onKeyUp = function (event) {
	var keycode = event.keyCode;
	switch (keycode) {
		// Shift
		case 16:
			this.mover.unalign();
			this.notify();
			break;
		default:	break;
	}
};

ms.exampleController.prototype.getExample = function() {
	return this.shapeMaker.exampleShape;
};
