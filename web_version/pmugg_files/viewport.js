ms.viewport = function(scale, centerX, centerY) {
	this.scale = scale;
	this.centerX = centerX;
	this.centerY = centerY;
	this.baseScale = scale;
	this.baseCenterX = centerX;
	this.baseCenterY = centerY;
	this.width  = ms.mainController.getCanvasWidth();
	this.height = ms.mainController.getCanvasHeight();
};

ms.viewport.prototype.setBaseScale = function(baseScale) {
	this.baseScale = baseScale;
};

ms.viewport.prototype.resize = function(w, h) {
	this.width = w;
	this.height = h;
};

ms.viewport.prototype.transformX = function(x) {
	return this.scale * (x - this.centerX) + this.width / 2;
};

ms.viewport.prototype.transformY = function(y) {
	return this.scale * (y - this.centerY) + this.height / 2;
};

ms.viewport.prototype.inverseTransform = function(screenPosition) {
	var x = (screenPosition.x - this.width  / 2) / this.scale + this.centerX;
	var y = (screenPosition.y - this.height / 2) / this.scale + this.centerY;
	return new ms.vec2(x, y);
};

ms.viewport.prototype.transform = function(position, offset, result) {
	var x = position.x;
	var y = position.y;
	if (offset) {
		x += offset.x;
		y += offset.y;
	}
	result.x = this.transformX(x);
	result.y = this.transformY(y);
};

ms.viewport.prototype.transform1 = function(position) {
	var result = new ms.vec2(0, 0);
	this.transform(position, ms.vec2.ORIGIN, result);
	return result;
};

ms.viewport.prototype.move = function(dx, dy) {
	this.centerX += dx / this.scale;
	this.centerY += dy / this.scale;
};

ms.viewport.prototype.zoom = function(dir, focusX, focusY) {
	var startScale = this.scale;
	var z;
	if (dir == 1) {
		z = ms.mainController.ZOOM_AMOUNT;
	} else {
		z = 1 / ms.mainController.ZOOM_AMOUNT;
	}
	this.scale *= z;
	if (this.scale < this.baseScale) {
		this.scale = this.baseScale;
		this.centerX = this.baseCenterX;
		this.centerY = this.baseCenterY;
	} else {
		var fx = this.centerX + (focusX - this.width / 2) / startScale;
		var fy = this.centerY + (focusY - this.height / 2) / startScale;
		this.centerX = fx + (this.centerX - fx) / z;
		this.centerY = fy + (this.centerY - fy) / z;
	}
};