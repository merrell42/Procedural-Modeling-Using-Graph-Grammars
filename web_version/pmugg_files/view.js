ms.view = function(shapeView, gridView, viewWebGL, view3D) {
	this.gridView = gridView;
	this.subViews = [];
	this.subViews[ms.view.modes.SHAPE] = shapeView;
	this.subViews[ms.view.modes.GRID] = gridView;
	this.subViews[ms.view.modes.VIEW3D] = view3D;
	if (!ms.mvp) {
		this.subViews[ms.view.modes.WEBGL] = viewWebGL;
	}
	this.mode = ms.view.modes.SHAPE;
};

ms.view.modes = {
	SHAPE: 0,
	GRID: 1,
	VIEW3D: 2,
	WEBGL: 3,
};

ms.view.prototype.setMode = function(mode, opt_controllerType) {
	if (mode == ms.view.modes.GRID && ms.globalSettings.get('Use WebGL')) {
		mode = ms.view.modes.WEBGL;
	}
	this.mode = mode;
	for (var i = 0; i < this.subViews.length; i++) {
		if (i == mode) {
			this.subViews[i].activate(opt_controllerType);
		} else {
			this.subViews[i].deactivate();
		}
	}
};

ms.view.prototype.getSubView = function(mode) {
	if (mode == ms.view.modes.GRID && ms.globalSettings.get('Use WebGL')) {
		mode = ms.view.modes.WEBGL;
	}
	return this.subViews[mode];
};

ms.view.prototype.getViewport = function() {
	return this.subViews[this.mode].getViewport();
};

ms.view.prototype.resize = function(width, height) {
	for (var i = 0; i < this.subViews.length; i++) {
		this.subViews[i].resize(width, height);
	}
};

ms.view.nullDriver = {
	getRenderables: function() { return []; },
	offset: function() { return ms.vec2.ORIGIN; },
};

ms.view.prototype.redraw = function (driver, opt_fullRedraw) {
	this.getSubView(this.mode).redraw(driver, opt_fullRedraw);
	// HACK: requiresShapeView should be removed.
	if (ms.highlightedElement && ms.highlightedElement.requiresShapeView && ms.highlightedElement.requiresShapeView() &&
		this.mode != ms.view.modes.SHAPE && this.mode != ms.view.modes.VIEW3D) {
		var shapeView = this.getSubView(ms.view.modes.SHAPE);
		shapeView.activateOver();
		shapeView.redraw(ms.view.nullDriver);
	}
};
