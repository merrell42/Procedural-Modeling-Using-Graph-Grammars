// The controller for the graph family tree.
ms.graphController = function(view, mainController) {
	this.view = view;
	this.mainController = mainController;
	this.viewport = null;
	this.cameraController = new ms.cameraController(view, this.notify.bind(this));
	this.classifier = new ms.classifier();
	this.familyTree = [];
};

ms.graphController.actionButtons = {
	CLASSIFY: 0,
};

ms.graphController.prototype.notify = function() {
	ms.timerG.start('Redraw');
	var expectedMode = this.classifier.is3D ? ms.view.modes.VIEW3D : ms.view.modes.SHAPE;
	if (this.view.mode != expectedMode) {
		this.view.setMode(expectedMode);
		this.viewport = this.view.getViewport();
		this.cameraController.activate();
	}
	this.view.redraw(this);
	ms.timerG.stop('Redraw');
};

ms.graphController.prototype.activate = function(opt_mode, opt_callback) {
	if (this.mainController.getExample().isEmpty()) {
		this.mainController.addTestObject();
	}
	this.view.setMode(ms.view.modes.SHAPE, ms.mainController.controllerTypes.GRAPH);
	this.mainController.setModeButtons([]);
	this.mainController.setActionButtons(ms.mvp ? [] : ['Classify']);
	this.viewport = this.view.getViewport();
	this.viewport.setBaseScale(0);
	this.cameraController.activate();
	ms.globalMutator = this;
	this.classify();
};

ms.graphController.prototype.applyAction = function(button) {
	var actionButtons = ms.graphController.actionButtons;
	if (button == actionButtons.CLASSIFY) {
		this.classify();
	}
};

ms.graphController.prototype.classify = function() {
	ms.edgeType.count = 0;
	ms.graph.count = 0;
	ms.graphEdge.count = 0;
	ms.counter.reset();
	var example = this.mainController.getExample();
	example.updateFaces();
	
	if (example.solution) {
		this.classifier.importSolution(example.solution);
		this.classifier.is3D = example.shapes[0].is3D;
	} else {		
		ms.timerG.start('Create Grammar');
		this.classifier.setShapes(example.getShapes());
		ms.timerG.stop('Create Grammar');
	}
	
	this.notify();
	console.log(ms.timerG.reportCurrent());
};

ms.graphController.prototype.getRenderables = function() {
	return [this.classifier.getFamilyTree()];
};

ms.graphController.prototype.offset = function() {
	return new ms.vec2(0, 0);
};

ms.graphController.prototype.onmousemove = function (event) {
	this.cameraController.onmousemove(event);
};

ms.graphController.prototype.onmouseup = function (event) {
	this.cameraController.onmouseup(event);
};

ms.graphController.prototype.onmousedown = function (event) {
	this.cameraController.onmousedown(event);
};


ms.graphController.prototype.onmouseout = function (event) {};

ms.graphController.prototype.onDoubleClick = function (event) {};

ms.graphController.prototype.onRightClick = function (event) {};

ms.graphController.prototype.onEscape = function (event) {};

ms.graphController.prototype.onKeyPress = function (event) {
	var keycode = event.keyCode;
	switch (keycode) {
		default:	break;
	}
};

ms.graphController.prototype.onKeyUp = function (event) {};
