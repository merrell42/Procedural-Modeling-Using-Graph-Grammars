ms.model = function(extents) {
	this.extents = extents;
	this.finished = false;
	this.nodeStats = null;
	this.bspTree = null;
	this.cells = ms.model.createArray(extents[0], extents[1], extents[2]);
	for (var x = 0; x < this.extents[0]; x++) {
		for (var y = 0; y < this.extents[1]; y++) {
			for (var z = 0; z < this.extents[2]; z++) {
				this.cells[x][y][z] = new ms.cell(x + ',' + y + ',' + z, new ms.vec2(x, y), false, this);
			}
		}
	}
	for (var x = 0; x < this.extents[0]; x++) {
		for (var y = 0; y < this.extents[1]; y++) {
			for (var z = 0; z < this.extents[2]; z++) {
				for (var dir = 0; dir < 3; dir++) {
					var direction = ms.cell.DIRECTIONS[2 * dir];
					var xd = x + direction[0];
					var yd = y + direction[1];
					var zd = z + direction[2];
					if (this.inBounds(xd, yd, zd)) {
						var cell0 = this.cells[x][y][z];
						var cellD = this.cells[xd][yd][zd];
						if (ms.globalSettings.get('Use Boundary Cells')) {
							switch(dir) {
								case 0: id = 'x';  break;
								case 1: id = 'y';  break;
								case 2: id = 'z';  break;
							}
							var boundaryCell = new ms.cell(id + x + ',' + y + ',' + z, new ms.vec2(x, y), true, this);
							cell0.addNeighbor(2 * dir, boundaryCell);
							boundaryCell.addNeighbor(2 * dir + 1, cell0);
							cellD.addNeighbor(2 * dir + 1, boundaryCell);
							boundaryCell.addNeighbor(2 * dir, cellD);
						} else {
							cell0.addNeighbor(2 * dir, cellD);
							cellD.addNeighbor(2 * dir + 1, cell0);
						}
					}
				}
			}
		}
	}
	
	// This is only for debugging.
	ms.globalModel = this;
};

ms.model.createArray = function(length) {
	var arr = new Array(length || 0), i = length;
	if (arguments.length > 1) {
		var args = Array.prototype.slice.call(arguments, 1);
		while (i--) {
			arr[length-1 - i] = ms.model.createArray.apply(this, args);
		}
	}
	return arr;
};

ms.model.prototype.setNodeStats = function(nodeStats) {
	this.nodeStats = nodeStats;
	this.bspTree = new ms.bspNode(nodeStats, null, []);
	nodeStats.setModel(this);
	for (var x = 0; x < this.extents[0]; x++) {
		for (var y = 0; y < this.extents[1]; y++) {
			for (var z = 0; z < this.extents[2]; z++) {
				this.cells[x][y][z].setNodeStats(nodeStats);
			}
		}
	}
};

ms.model.prototype.getElements = function(type) {
	return this.nodeStats ? this.nodeStats.getElements(type) : [];
}

ms.model.prototype.getFaces = function() {
	return this.nodeStats ? this.nodeStats.getElements('face') : [];
};

ms.model.prototype.getLines = function() {
	return this.nodeStats ? this.nodeStats.getElements('line') : [];
};

ms.model.prototype.getVertices = function() {
	return this.nodeStats ? this.nodeStats.getElements('vertex') : [];
};

ms.model.prototype.getRenderables = function() {
	return this.getFaces().concat(this.getLines());
};

ms.model.prototype.inBounds = function(x, y, z) {
	return (0 <= x) && (x < this.extents[0]) &&
	       (0 <= y) && (y < this.extents[1]) &&
	       (0 <= z) && (z < this.extents[2]);
};

ms.model.prototype.getValue = function(x, y, z) {
	return this.inBounds(x, y, z) ? this.cells[x][y][z].getValue() : null;
};

ms.model.prototype.getCell = function(x, y, z) {
	z = z || 0;
	return this.inBounds(x, y, z) ? this.cells[x][y][z] : null;
};

ms.model.prototype.cellFromPosition = function(p) {
	var cellX = Math.floor(p.x);
	var cellY = Math.floor(p.y);
	if (this.inBounds(cellX, cellY, 0)) {		
		return this.getCell(cellX, cellY);
	} else {
		return new ms.cell('origin', new ms.vec2(0, 0), false, this);
	}
};

ms.model.prototype.getExtents = function() {
	return this.extents;
};

ms.model.prototype.getBounds = function(dim) {
	return [1e-5, this.extents[dim] - 1e-5];
};

// Cast a ray to the left of the position p. Return the face of the first intersection.
// Similar to ms.intersector.lineFaceIntersect.
ms.model.prototype.rayCastLeft = function(p) {
	var y = Math.floor(p.y);
	for (var x = Math.floor(p.x); x >= 0; x--) {
		var cell = this.cells[x][y][0];
		var states = cell.getActiveStates();
		var p0 = new ms.vec2(0, p.y);
		var result = null;
		states.forEach(function(state) {
			var endpoints = state.getLine().getEndpoints();
			var state0 = endpoints[0].getPosition();
			var state1 = endpoints[1].getPosition();
			var intersection = ms.intersector.intersect(p0, p, state0, state1, 0);
			if (intersection) {
				p0 = intersection;
				result = state0.y > state1.y ? endpoints[0] : endpoints[1];
			}
		});
		// x must reach the cell in which the intersection occurs. Since there may be states
		// spanning multiple cells that are not the first intersection.
		if (result && Math.floor(p0.x) == x) {
			return {position: p0, endpoint: result};
		}
	}
	return null;
};

ms.model.prototype.print = function() {
	var extents = this.getExtents();
	var output = '';
	for (var z = 0; z < extents[2]; z++) {
		for (var x = 0; x < extents[0]; x++) {
			var rowString = '';
			for (var y = 0; y < extents[1]; y++) {
				var states = this.getCell(x, y, z).getActiveStates();
				var cellArray = [];
				for (var i = 0; i < states.length; i++) {
					cellArray.push(states[i].getValue().getKey());
				}
				var cellString = cellArray.sort().toString();
				while (cellString.length < 7) {
					cellString = cellString + ' ';
					if (cellString.length < 7) {
					cellString = ' ' + cellString;
					}
				}
				output += cellString;
			}
			output += '\n';
		}
		output += '\n';
	}
	return output;
};

ms.model.prototype.printValues = function() {
	var extents = this.getExtents();
	var output = '';
	for (var z = 0; z < extents[2]; z++) {
		for (var x = 0; x < extents[0]; x++) {
			var rowString = '';
			for (var y = 0; y < extents[1]; y++) {
				var cellString = this.getCell(x, y, z).getRemovedStates().length.toString();
				while (cellString.length < 7) {
					cellString = cellString + ' ';
					if (cellString.length < 7) {
					cellString = ' ' + cellString;
					}
				}
				output += cellString;
			}
			output += '\n';
		}
		output += '\n';
	}
	return output;
};

ms.model.prototype.leftBoundary = function() {
	return 1.1;
};

ms.model.prototype.rightBoundary = function() {
	return this.extents[0] - 1.1;
};

ms.model.prototype.getFinished = function() {
	return this.finished;
};

ms.model.prototype.setFinished = function(finished) {
	this.finished = finished;
};

ms.model.prototype.getBspTree = function() {
	return this.bspTree;
};
