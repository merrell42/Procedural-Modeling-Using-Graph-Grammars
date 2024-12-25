// Points
ms.point = function(x, y, opt_isControl) {
	this.position = new ms.vec2(x, y);
	this.edges = [];
	this.selected = true;
	this.hovered = false;
	this.visible = true;
	this.parent = null;

	// If the control scale is non-zero then the child control points are locked
	// together. The vectors between each child and this point are colinear. The
	// second vector is equal to the first vector scaled by this quantity.
	this.controlScale = 0;
	this.isControl = opt_isControl || false;
};

ms.point.prototype.lockControlPoints = function() {
	alert('Lock Control Points');
};

ms.point.prototype.isLarge =  function () {
	return !this.isControl;
};

ms.point.prototype.getColor =  function () {
	if (this.isControl) {
		if (this.hovered) {
			return ms.shapeView.HOVERED_CONTROL_COLOR;
		} else {
			return ms.shapeView.CONTROL_POINT_COLOR;
		}
	} else if (this.selected) {
		return ms.shapeView.SELECTED_COLOR;
	} else {
		if (this.hovered) {
			return ms.shapeView.HOVERED_COLOR;
		} else {
			return ms.shapeView.DEFAULT_COLOR;
		}
	}
};

ms.point.prototype.getGroup = function() {
	return this;
};

ms.point.prototype.getVertex = function() {
	return this;
};

ms.point.prototype.getVertices = function() {
	return [];
};

ms.point.prototype.getPosition = function() {
	return this.position;
};

ms.point.prototype.getX = function() {
	return this.position.x;
};

ms.point.prototype.getY = function() {
	return this.position.y;
};

ms.point.prototype.distance = function(p) {
	return this.position.distance(p.getPosition());
};

ms.point.prototype.getSelected = function() {
	return this.selected;
};

ms.point.prototype.hover = function() {
	var changed = !this.hovered;
	this.hovered = true;
	return changed;
};

ms.point.prototype.unhover = function() {
	var changed = this.hovered;
	this.hovered = false;
	return changed;
};

ms.point.prototype.select = function() {
	this.selected = true;
};

ms.point.prototype.deselect = function() {
	this.selected = false;
};

ms.point.prototype.getVisible = function() {
	return this.visible;
};

ms.point.prototype.copy = function() {
	return new ms.point(this.position.x, this.position.y);
};

ms.point.prototype.addEndpoint = function(endpoint) {};

ms.point.prototype.removeEndpoint = function(endpoint) {};

ms.point.prototype.move = function(dx, dy, opt_force) {
	if (this.selected || opt_force) {
		this.position.x += dx;
		this.position.y += d;y
	}
};

ms.point.prototype.controlPointMove = function(dx, dy) {
	if (this.isControl && this.selected) {
		this.position.x += dx;
		this.position.y += dy;
		var controlScale = this.parent.controlScale;
		if (controlScale) {
			var firstChild = (this.parent.children[0] == this);
			var otherChild = this.parent.children[firstChild ?  1 : 0];
			var scale = firstChild ? 1 / controlScale : controlScale;
			otherChild.position.x += scale * dx;
			otherChild.position.y += scale * dy;
			var d1 = this.parent.distance(this);
			var d2 = this.parent.distance(otherChild);
			window.console.log(d1 + ' ' + d2 + ' ' + d2 / d1 + ' ' + scale + ' ' + firstChild);
		}
	}
};

ms.point.prototype.move2 = function(v) {
	this.position.add(v);
};
