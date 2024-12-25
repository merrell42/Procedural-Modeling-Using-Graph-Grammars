ms.draggableLine = function(u, posFromBrush, brushFromPos) {
	this.u = u;
	this.v = new ms.vec2(-u.y, u.x);
	this.posFromBrush = posFromBrush;
	this.brushFromPos = brushFromPos;
	this.selected = false;
};

ms.draggableLine.prototype.isSnappable = function() {
	return false;
};

ms.draggableLine.LENGTH = 30;

ms.draggableLine.prototype.draw = function(view, offset) {
	var center = this.posFromBrush();
	var p0 = this.v.copy().scale( ms.draggableLine.LENGTH / 2).add(center);
	var p1 = this.v.copy().scale(-ms.draggableLine.LENGTH / 2).add(center);
	
	view.context.lineWidth = 3;
	view.context.beginPath();
	// var color = this.selected ? ms.shapeView.HOVERED_COLOR.fill : 
	var color = this.selected ? ms.shapeView.SELECTED_COLOR.fill : ms.shapeView.DEFAULT_COLOR.fill
	view.drawLine(color, p0, p1, offset, {});
	view.context.stroke();
	
};

ms.draggableLine.prototype.getSelected = function() {
	return this.selected;
};

// Returns true is the position is close. This is used for selection.
ms.draggableLine.prototype.isClose = function(query, scale) {
	var position = this.posFromBrush();
	if (!position) {
		return false;
	}
	var pu = position.dot(this.u);
	var pv = position.dot(this.v);
	var qu = query.dot(this.u);
	var qv = query.dot(this.v);
	return Math.abs(pu - qu) < ms.shapeSet.NEAR_RADIUS / scale && Math.abs(pv - qv) < ms.draggableLine.LENGTH / 2;
};

ms.draggableLine.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.DRAGGABLE;
};

ms.draggableLine.prototype.select = function() {
	this.selected = true;
};

ms.draggableLine.prototype.deselect = function() {
	this.selected = false;
};

ms.draggableLine.prototype.move = function(dx, dy) {
	var position = this.posFromBrush();
	var movement = new ms.vec2(dx, dy);
	var du = movement.dot(this.u);
	var newPosition = position.copy().add(this.u.copy().scale(du));
	this.brushFromPos(newPosition);
};
