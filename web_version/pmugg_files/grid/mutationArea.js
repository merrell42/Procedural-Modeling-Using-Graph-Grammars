ms.mutationArea = function(model, stats) {
	this.lowerExtent = [0, 0, 0];
	this.upperExtent = model.getExtents();
	this.model = model;
	this.borders = [];
	this.stats = stats;
};

ms.mutationArea.prototype.setExtents = function(lowerExtent, size) {
	this.lowerExtent = lowerExtent;
	this.upperExtent = [lowerExtent[0] + size[0], lowerExtent[1] + size[1], lowerExtent[2] + size[2]];
};

ms.mutationArea.prototype.isInside = function(v) {
	return (this.lowerExtent[0] <= v.x) && (v.x <= this.upperExtent[0]) &&
	       (this.lowerExtent[1] <= v.y) && (v.y <= this.upperExtent[1]);
};

ms.mutationArea.prototype.apply = function(f) {
	for (var x = this.lowerExtent[0]; x < this.upperExtent[0]; x++) {
		for (var y = this.lowerExtent[1]; y < this.upperExtent[1]; y++) {
			for (var z = this.lowerExtent[2]; z < this.upperExtent[2]; z++) {
				var cell = this.model.getCell(x, y, z);
				f(cell);
				if (ms.globalSettings.get('Use Boundary Cells')) {
					for (var i = 0; i < ms.cell.DIMS; i++) {
						if (cell.getNeighbor(2 * i)) {
							f(cell.getNeighbor(2 * i));
						}
					}
				}
			}
		}
	}
};

ms.mutationArea.prototype.pickCell = function() {
	var x = ms.random(this.upperExtent[0] - this.lowerExtent[0]) + this.lowerExtent[0];
	var y = ms.random(this.upperExtent[1] - this.lowerExtent[1]) + this.lowerExtent[1];
	var z = ms.random(this.upperExtent[2] - this.lowerExtent[2]) + this.lowerExtent[2];
	return this.model.getCell(x, y, z);
};

ms.mutationArea.prototype.save = function() {
	this.apply(function(cell) {
		cell.save();
	});
};

ms.mutationArea.prototype.free = function() {
	this.apply(function(cell) {
		cell.save();
		cell.free();
	});
};

ms.mutationArea.prototype.restrain = function() {
	this.apply(function(cell) {
		cell.restrain();
	});
};

ms.mutationArea.prototype.restore = function() {
	this.apply(function(cell) {
		cell.restore();
	});
	this.apply(function(cell) {
		cell.connect();
	});
};
