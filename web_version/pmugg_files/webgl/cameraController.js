// The controller for camera movements in the 3D grid.
ms.cameraController = function(view, notifyFunc) {
	this.view = view;
	this.notify = notifyFunc;
	this.viewport = null;
	this.mouseDown = false;
	this.mouseX = 0;
	this.mouseY = 0;
};

ms.cameraController.prototype.activate = function() {
	this.viewport = this.view.getViewport();
};

ms.cameraController.prototype.onmousemove = function (event) {
	if (this.mouseDown) {
		var dx = event.x - this.mouseX;
		var dy = event.y - this.mouseY;
		this.mouseX = event.x;
		this.mouseY = event.y;
		this.viewport.orbit(dx, dy);
		this.notify();
	}
};

ms.cameraController.prototype.onmouseup = function (event) {
	this.mouseDown = false;
};

ms.cameraController.prototype.onmousedown = function (event) {
	this.mouseDown = true;
	this.mouseX = event.x;
	this.mouseY = event.y;
	
	var margin = ms.generatedController.BODY_MARGIN;
	this.viewport.setInterest(event.x - margin, event.y - margin);
};
