ms.mover = function(exampleShape) {
	this.actualMovement = new ms.vec2(0, 0);
	this.snappedMovement = new ms.vec2(0, 0);
	this.axisAligned = false;
	this.movee = null;
	this.exampleShape = exampleShape;

	this.tmpVec = new ms.vec2(0, 0);
};

ms.mover.snapDistance = 10;
ms.mover.snapDistance2 = ms.mover.snapDistance * ms.mover.snapDistance;

ms.mover.prototype.startMove = function(movee, opt_movement) {
	this.movee = movee;
	if (!opt_movement) {
		this.actualMovement.x = 0;
		this.actualMovement.y = 0;
		this.snappedMovement.x = 0;
		this.snappedMovement.y = 0;
	} else {
		this.actualMovement.x = opt_movement.x;
		this.actualMovement.y = opt_movement.y;
		this.snappedMovement.x = opt_movement.x;
		this.snappedMovement.y = opt_movement.y;
		this.move(0, 0);
	}
};

ms.mover.prototype.finishMove = function() {
	if (this.movee.selectType && this.movee.selectType() == ms.shapeMaker.SelectableTypes.EDGE) {
		return;
	}
	if (this.movee && this.movee.isSnappable()) {
		this.exampleShape.mergeVertexGroups(this.movee);
	}
};

ms.mover.prototype.align = function(movee, opt_movement) {
	if (!this.axisAligned) {
		this.axisAligned = true;
		this.startMove(movee, opt_movement);
	}
};

ms.mover.prototype.unalign = function() {
	this.axisAligned = false;
	this.move(0, 0);
};

ms.mover.prototype.move = function(dx, dy, shiftKey) {
	if (this.movee.selectType && this.movee.selectType() == ms.shapeMaker.SelectableTypes.EDGE) {
		return;
	}
	this.actualMovement.x += dx;
	this.actualMovement.y += dy;

	var x = this.actualMovement.x;
	var y = this.actualMovement.y;
	if (!this.movee.isSnappable() || !ms.globalSettings.get('Snap Grid')){
		this.movee.move(dx, dy, shiftKey);
	} else if (this.axisAligned) {
		if (Math.abs(this.actualMovement.x) > Math.abs(this.actualMovement.y)) {
			y = 0;	
		} else {
			x = 0;
		}

		var dx = x - this.snappedMovement.x;
		var dy = y - this.snappedMovement.y;

		this.movee.move(dx, dy);
		this.snappedMovement.x = x;
		this.snappedMovement.y = y;
	} else {
		var dx = x - this.snappedMovement.x;
		var dy = y - this.snappedMovement.y;

		this.movee.move(dx, dy);

		var spacing = ms.globalSettings.get('Snap Grid Spacing') / 2;
		var d = spacing * spacing;
		var vx = 0;
		var vy = 0;
		var movingPositions = this.movee.getSelectedPositions();
		var movingVertices = this.movee.getSelectedVertices();
		var allPositions = this.exampleShape.getAllPositions();
		var allVertices = this.exampleShape.getAllVertices();
		var snappedToVertex = false;
		for (var i = 0; i < movingPositions.length; i++) {
			var a = movingPositions[i];
			for (var j = 0; j < allPositions.length; j++) {
				var b = allPositions[j];
				if (movingVertices[i] != allVertices[j]) {
					this.tmpVec.x = b.x - a.x;
					this.tmpVec.y = b.y - a.y;
					var d0 = this.tmpVec.length2();
					if (d0 < d) {
						d = d0;
						vx = this.tmpVec.x;
						vy = this.tmpVec.y;
						snappedToVertex = true;
					}
				}
			}
		}
		if (!snappedToVertex && ms.globalSettings.get('Snap Grid')) {
			var spacing = ms.globalSettings.get('Snap Grid Spacing')
			var ax = movingPositions[0].x;
			var ay = movingPositions[0].y;
			var sx = spacing * Math.round(ax / spacing);
			var sy = spacing * Math.round(ay / spacing);
			vx = sx - ax;
			vy = sy - ay;
		}		
		this.movee.move(vx, vy);
		this.snappedMovement.x = x + vx;
		this.snappedMovement.y = y + vy;
	}
};
