setup = function() {
	var canvas = document.getElementById("canvas1");
	var shapeView = new ms.shapeView(canvas);
	var gridCanvas = document.getElementById("grid-canvas");
	var offscreenCanvas = document.getElementById("canvas2");
	var canvas3d = document.getElementById("canvas-3d");
	var gridView = new ms.gridView(gridCanvas, canvas, offscreenCanvas);
	var view3d = new ms.view3d(canvas3d);
	if (!ms.mvp) {
		var webglCanvas = document.getElementById("webgl-canvas");
		var viewWebGL = new ms.viewWebGL(webglCanvas, canvas);
	}
	
	var view = new ms.view(shapeView, gridView, viewWebGL, view3d);
	
	var width =  window.innerWidth - 330;
	var height =  window.innerHeight - 100;
	view.resize(width, height);

	var screenSaver = new ms.screenSaver(gridCanvas, offscreenCanvas);
	var controller = new ms.mainController(view, screenSaver);
	var canvases = [canvas, gridCanvas, canvas3d];
	if (!ms.mvp) {
		canvases.push(webglCanvas);
	}
	window.controller = controller;

	var onmousemoveHandler = controller.onmousemove.bind(controller);
	var onmousedownHandler = controller.onmousedown.bind(controller);
	var onmouseupHandler = controller.onmouseup.bind(controller);
	var onmouseoutHandler = controller.onmouseout.bind(controller);
	var onmousewheelHandler = controller.onmousewheel.bind(controller);
	var onDoubleClickHandler = controller.onDoubleClick.bind(controller);
	var onRightClickHandler = controller.onRightClick.bind(controller);
	for (var i = 0; i < canvases.length; i++) {
		canvases[i].addEventListener("mousemove", onmousemoveHandler);
		canvases[i].addEventListener("mousedown", onmousedownHandler);
		canvases[i].addEventListener("mouseup", onmouseupHandler);
		canvases[i].addEventListener("mouseout", onmouseoutHandler);
		canvases[i].addEventListener("mousewheel", onmousewheelHandler);
		canvases[i].addEventListener("dblclick", onDoubleClickHandler);
		canvases[i].addEventListener("contextmenu", onRightClickHandler);
	}

	var onKeyPressHandler = controller.onKeyPress.bind(controller);
	window.addEventListener("keydown", onKeyPressHandler);
	
	var onKeyUpHandler = controller.onKeyUp.bind(controller);
	window.addEventListener("keyup", onKeyUpHandler);

	ms.globalSettings.draw(document.getElementById('settings-container'));
};
