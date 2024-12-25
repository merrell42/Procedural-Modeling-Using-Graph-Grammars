// The main application controller. Handles keyboard and mouse events.
ms.mainController = function(view, screenSaver) {
	this.currentController = null;

	this.exampleShape = new ms.exampleShape();
	var decorationModel = new ms.decorationModel(this.exampleShape);
	
	var decorationView = new ms.decorationView(document.getElementById('decoration-container'), decorationModel);
	this.brushSelector = null;
	if (!ms.mvp) {
		var brushSelectorContainer = document.getElementById('brush-selector');
		this.brushSelector = new ms.brushSelector(brushSelectorContainer, decorationModel);
	}
	var shapeMaker = new ms.shapeMaker(this.exampleShape, decorationModel);
	shapeMaker.register(view);
	this.modeControllers = [
			new ms.objectController(view, shapeMaker, decorationModel, this),
			// new ms.exampleController(view, shapeMaker, this.exampleShape, decorationModel, this),
			new ms.graphController(view, this),
			new ms.generatedController(view, this),
	];
	this.brushSelector && this.brushSelector.setController(this);
	this.modeElements = [];
	this.state = ms.mainController.stateTypes.DEFAULT;
	this.prevMouse = null;
	
	var testContainer = document.getElementById('test-container');
	this.testRunner = new ms.testRunner(testContainer, this.exampleShape, decorationModel, screenSaver, this);

	var controllerSwitcher = document.getElementById('controller-switcher');
	this.controllerElements = controllerSwitcher.children;
	for (var i = 0; i < this.controllerElements.length; i++) {
		var clickHandler = this.updateController.bind(this, i, null, null);
		this.controllerElements[i].addEventListener("click", clickHandler, false);
	}
	this.updateController(ms.mainController.controllerTypes.OBJECT);
	// This imports the shape.
	this.testRunner.add();
	// There is no selected shape until it is imported.
	// This is needed to keep the brush view is up to date. And select the first shape.
	 this.updateController(ms.mainController.controllerTypes.OBJECT);
};

ms.mainController.controllerTypes = {
	OBJECT: 0,
	// EXAMPLE: 1,
	GRAPH: 1,
	GENERATED: 2,
};

ms.mainController.stateTypes = {
	DEFAULT: 0,
	PAN: 1
};

ms.mainController.ZOOM_AMOUNT = 1.5;

ms.mainController.prototype.notify = function() {
	this.currentController.notify();
};

ms.mainController.prototype.updateController = function(controllerType, opt_mode, opt_callback) {
	this.currentController = this.modeControllers[controllerType];
	for (var i = 0; i < this.controllerElements.length; i++) {
        this.controllerElements[i].className = (i == controllerType) ? 'active-mode-button' : 'inactive-mode-button';
	}
	this.currentController.activate(opt_mode, opt_callback);
};

ms.mainController.prototype.setModeButtons = function(modeButtons) {
	var modeSwitcher = document.getElementById('mode-switcher');
	while (modeSwitcher.firstChild) {
		modeSwitcher.removeChild(modeSwitcher.firstChild);
	}
	this.modeElements = [];
	for (var i = 0; i < modeButtons.length; i++) {
		var clickHandler = this.updateMode.bind(this, i);
		var newChild = document.createElement('span');
		modeSwitcher.appendChild(newChild);
		newChild.addEventListener("click", clickHandler, false);
		newChild.innerHTML = modeButtons[i];
		this.modeElements.push(newChild);
	}
};

ms.mainController.prototype.setActionButtons = function(actionButtons, opt_actionIcons) {
    var actionSwitcher = document.getElementById('action-container');
    while (actionSwitcher.firstChild) {
        actionSwitcher.removeChild(actionSwitcher.firstChild);
    }
    for (var i = 0; i < actionButtons.length; i++) {
        var clickHandler = this.applyAction.bind(this, i);
        var icon = opt_actionIcons && opt_actionIcons[i];
        if (icon) {
            var newChild = document.createElement('img');
            newChild.src = icon;
            newChild.className = 'icon-button';
        } else {
            var newChild = document.createElement('span');
            newChild.innerHTML = actionButtons[i];
            newChild.className = 'inactive-mode';
        }
        actionSwitcher.appendChild(newChild);
        newChild.addEventListener("click", clickHandler, false);
    }
};

ms.mainController.prototype.setActionEnabled = function(enabled) {
    var actionSwitcher = document.getElementById('action-container');
    for (var i = 0; i < enabled.length; i++) {
        actionSwitcher.childNodes[i].className = enabled[i] ? 'enabled-icon-button' : 'disabled-icon-button';
    }
};

ms.mainController.prototype.setBrushButtons = function() {
	var actionSwitcher = document.getElementById('action-container');
	while (actionSwitcher.firstChild) {
		actionSwitcher.removeChild(actionSwitcher.firstChild);
	}
	for (var i = 0; i < actionButtons.length; i++) {
		var clickHandler = this.applyAction.bind(this, i);
		var newChild = document.createElement('span');
		actionSwitcher.appendChild(newChild);
		newChild.innerHTML = actionButtons[i];
		newChild.addEventListener("click", clickHandler, false);
		newChild.className = 'inactive-mode';
	}
};

ms.mainController.prototype.updateMode = function(mode) {
	for (var i = 0; i < this.modeElements.length; i++) {
		this.modeElements[i].className = (i == mode) ? 'active-mode' : 'inactive-mode';
	}
	this.currentController.updateMode(mode);
};

ms.mainController.prototype.applyAction = function(action) {
	this.currentController.applyAction(action);
};

ms.mainController.prototype.addTestObject = function() {
	this.testRunner.add();
};

ms.mainController.prototype.onmousemove = function(event) {
	if (this.state == ms.mainController.stateTypes.PAN) {
		var dx = event.x - this.prevMouse.x;
		var dy = event.y - this.prevMouse.y;
		this.currentController.viewport.move(-dx, -dy);
		this.notify();
		this.prevMouse.x = event.x;
		this.prevMouse.y = event.y;
	} else {
		this.currentController.onmousemove(event);
	}
};

ms.mainController.prototype.onmouseup = function(event) {
	if (this.state == ms.mainController.stateTypes.PAN) {
		this.state = ms.mainController.stateTypes.DEFAULT;
	} else {
		this.currentController.onmouseup(event);
	}
};

ms.mainController.prototype.onmousedown = function(event) {
	if (event.button == 1) {
		this.state = ms.mainController.stateTypes.PAN;
		this.prevMouse = new ms.vec2(event.x, event.y);
	} else {
		this.currentController.onmousedown(event);
	}
};

ms.mainController.prototype.onmousewheel = function(event) {
	if (event.wheelDelta > 0) {
		this.zoom(1, event.x, event.y);
	} else {
		this.zoom(-1, event.x, event.y);
	}
};

ms.mainController.prototype.zoom = function(dir, focusX, focusY) {
	var margin = ms.generatedController.BODY_MARGIN;
	focusX -= margin;
	focusY -= margin;
	this.currentController.view.getViewport().zoom(dir, focusX, focusY);
	this.notify();
};

ms.mainController.prototype.onmouseout = function(event) {
	if (this.state == ms.mainController.stateTypes.DEFAULT) {
		this.currentController.onmouseout(event);
	}
};

ms.mainController.prototype.onDoubleClick = function(event) {
	this.currentController.onDoubleClick(event);
};

ms.mainController.prototype.onRightClick = function(event) {
	this.currentController.onRightClick(event);
};

ms.mainController.prototype.onKeyPress = function(event) {
	var keycode = event.keyCode;
	switch (keycode) {
		case 83:
			this.updateController(ms.mainController.controllerTypes.GENERATED);
			break;
		// Plus
		case 187:
			this.zoom(1, ms.mainController.getCanvasWidth() / 2, ms.mainController.getCanvasHeight() / 2);
			break;
		// Minus
		case 189:
			this.zoom(-1, ms.mainController.getCanvasWidth() / 2, ms.mainController.getCanvasHeight() / 2);
			break;
		default:	
			this.currentController.onKeyPress(event);
			break;
	}
};

ms.mainController.prototype.onKeyUp = function (event) {
	var keycode = event.keyCode;
	switch (keycode) {
		default:
			this.currentController.onKeyUp(event);
			break;
	}
};

ms.mainController.prototype.getExample = function() {
	return this.exampleShape;
};

ms.mainController.getCanvasWidth = function() {
	return document.getElementById('canvas1').width;
};

ms.mainController.getCanvasHeight = function() {
	return document.getElementById('canvas1').height;
};
