ms.draggablePoint = function(posFromBrush, brushFromPos, anchorFromBrush, brushFromAnchor) {
	this.posFromBrush = posFromBrush;
	this.brushFromPos = brushFromPos;
	this.anchorFromBrush = anchorFromBrush;
	this.brushFromAnchor = brushFromAnchor;
	this.selected = false;
};

ms.draggablePoint.prototype.isVertex = function() {
	return false;
};

ms.draggablePoint.prototype.isSnappable = function() {
	return false;
};

ms.draggablePoint.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.DRAGGABLE;
};

ms.draggablePoint.RADIUS = 10;

ms.draggablePoint.prototype.draw = function(view, offset) {
	var r = ms.draggablePoint.RADIUS;
	var anchor = this.anchorFromBrush();
	var px0 = anchor.copy();
	var px1 = anchor.copy();
	var py0 = anchor.copy();
	var py1 = anchor.copy();
	px0.x -= r;
	px1.x += r;
	py0.y -= r;
	py1.y += r;
	
	view.context.lineWidth = 2;
	view.context.beginPath();
	var color = this.selected ? ms.shapeView.SELECTED_COLOR.fill : ms.shapeView.DEFAULT_COLOR.fill
	view.drawLine(color, px0, px1, offset, {});
	view.context.stroke();
	view.drawLine(color, py0, py1, offset, {});
	view.context.stroke();
	
};

ms.draggablePoint.prototype.getSelected = function() {
	return this.selected;
};

// Returns true is the position is close. This is used for selection.
ms.draggablePoint.prototype.isClose = function(query, scale) {
	var anchor = this.anchorFromBrush();
	return (anchor && query.distance2(anchor) < ms.shapeSet.NEAR_RADIUS2 / (scale * scale));
};

ms.draggablePoint.prototype.select = function() {
	this.selected = true;
};

ms.draggablePoint.prototype.deselect = function() {
	this.selected = false;
};

ms.draggablePoint.prototype.move = function(dx, dy, ctrlKey) {
	if (!ctrlKey) {
		var position = this.posFromBrush(ctrlKey);
		var movement = new ms.vec2(dx, dy);
		var newPosition = position.copy().add(movement);
		this.brushFromPos(newPosition, ctrlKey);
	}
	var position = this.anchorFromBrush(ctrlKey);
	var movement = new ms.vec2(dx, dy);
	var newPosition = position.copy().add(movement);
	this.brushFromAnchor(newPosition, ctrlKey);
};
