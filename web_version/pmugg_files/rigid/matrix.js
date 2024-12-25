ms.matrix = function(opt_format, value) {
	this.ccs = null;
	this.scatter = null;
	this.full = null;
	this.lup = null;
	if (opt_format == 'ccs') {
		this.ccs = value;
	} else if (opt_format == 'scatter') {
		this.scatter = value;
	} else if (opt_format == 'full') {
		this.full = value;
	} else if (!!opt_format) {
		ms.alert('Incorrect format name');
	}
};

ms.matrix.prefferedFormat = 'ccs';

ms.matrix.prototype.addValue = function(row, column, value) {
	if (!this.scatter) {
		this.scatter = [[], [], []];
	}
	this.scatter[0].push(row);
	this.scatter[1].push(column);
	this.scatter[2].push(value);
};

ms.matrix.prototype.asCcs = function() {
	if (!this.ccs) {
		if (this.scatter) {
			this.ccs = numeric.ccsScatter(this.scatter);
		} else if (this.full) {
			this.ccs = numeric.ccsSparse(this.full);
		} else {
			ms.alert('Matrix is empty in asCcs.');	
		}
	}
	return this.ccs;
};

ms.matrix.prototype.asFull = function() {
	if (!this.full) {
		if (this.ccs) {
			this.full = numeric.ccsFull(this.ccs);
		} else if (this.scatter) {
			this.ccs = this.asCcs();
			this.full = numeric.ccsFull(this.ccs);
		} else {
			ms.alert('Matrix is empty in asCcs.');	
		}
	}
	return this.full;
};

ms.matrix.prototype.isEmpty = function() {
	if (this.full) {
		return this.full.length == 0 || this.full[0].length == 0;
	} else if (this.ccs) {
		return this.ccs[1].length == 0 && this.ccs[2].length == 0;
	} else if (this.scatter) {
		return this.scatter[0].length == 0;
	} else {
		return true;
	}
};

ms.matrix.prototype.dot = function(B) {
	if (ms.matrix.prefferedFormat == 'ccs') {
		var result = numeric.ccsDot(this.asCcs(), B.asCcs());
		return new ms.matrix('ccs', result);
	} else if (ms.matrix.prefferedFormat == 'full') {
		var result = numeric.dot(this.asFull(), B.asFull());
		return new ms.matrix('full', result);
	}
};

ms.matrix.prototype.minus = function(B) {
	if (ms.matrix.prefferedFormat == 'ccs') {
		var result = numeric.ccssub(this.asCcs(), B.asCcs());
		return new ms.matrix('ccs', result);
	} else if (ms.matrix.prefferedFormat == 'full') {
		var result = numeric.sub(this.asFull(), B.asFull());
		return new ms.matrix('full', result);
	}
};

ms.matrix.prototype.add = function(B) {
	if (ms.matrix.prefferedFormat == 'ccs') {
		var result = numeric.ccsadd(this.asCcs(), B.asCcs());
		return new ms.matrix('ccs', result);
	} else if (ms.matrix.prefferedFormat == 'full') {
		var result = numeric.add(this.asFull(), B.asFull());
		return new ms.matrix('full', result);
	}
};

// Solve Ax = B. Where this is A.
ms.matrix.prototype.solve = function(B) {
	if (ms.matrix.prefferedFormat == 'ccs') {
		var result = numeric.ccsLUPSolve(this.asLup(), B.asCcs());
		return new ms.matrix('ccs', result);
	} else if (ms.matrix.prefferedFormat == 'full') {
		var result = numeric.solve(this.asFull(), B.asFull());
		return new ms.matrix('full', numeric.transpose([result]));
	}
};

ms.matrix.prototype.asLup = function() {
	if (!this.lup) {
		this.lup = numeric.ccsLUP(this.asCcs());
	}
	return this.lup;
};

// Transpose the matrix
ms.matrix.prototype.T = function() {
	if (this.scatter) {
		var transposed = [this.scatter[1].slice(), this.scatter[0].slice(), this.scatter[2]];
		return new ms.matrix('scatter', transposed);
	} else if (this.full) {
		return new ms.matrix('full', numeric.transpose(this.full));
	} else {
		return new ms.matrix('full', numeric.transpose(this.asFull()));
	}
};

// Create a random diagonal matrix.
ms.matrix.givenDiagonal = function(size, value) {
	var result = new ms.matrix();
	for (var i = 0; i < size; i++) {
		result.addValue(i, i, value);
	}
	return result;
};

// Create a random diagonal matrix.
ms.matrix.randomDiagonal = function(w) {
	var matrix = [];
	for (var row = 0; row < w; row++) {
		var newRow = [];
		for (var col = 0; col < w; col++) {
			if (row == col) {
				newRow.push(10 * Math.random());
			} else {
				newRow.push(0);
			}
		}
		matrix.push(newRow);
	}
	return new ms.matrix('full', matrix);
};

// Create a random diagonal matrix.
ms.matrix.identity = function(w) {
	var matrix = [];
	for (var row = 0; row < w; row++) {
		var newRow = [];
		for (var col = 0; col < w; col++) {
			if (row == col) {
				newRow.push(1);
			} else {
				newRow.push(0);
			}
		}
		matrix.push(newRow);
	}
	return new ms.matrix('full', matrix);
};

// Transpose the matrix
ms.matrix.random = function(w, h, scale) {
	var matrix = [];
	for (var row = 0; row < w; row++) {
		var newRow = [];
		for (var col = 0; col < h; col++) {
			newRow.push(scale * Math.random());
		}
		matrix.push(newRow);
	}
	return new ms.matrix('full', matrix);
};

ms.matrix.prototype.copy = function() {
	this.asFull();
	var copy = [];
	for (var row = 0; row < this.full.length; row++) {
		copy.push(this.full[row].slice());
	}
	return new ms.matrix('full', copy);
};

ms.matrix.prototype.spliceColumns = function(start, length) {
	var newMatrix = [];
	for (var row = 0; row < this.full.length; row++) {
		newMatrix.push(this.full[row].splice(start, length));
	}
	this.scatter = null;
	this.ccs = null;
	return new ms.matrix('full', newMatrix);
};

ms.matrix.combineColumns = function(columns) {
	var newMatrix = new ms.matrix();
	for (var col = 0; col < columns.length; col++) {
		var column = columns[col].asFull();
		for (var row = 0; row < column.length; row++) {
			newMatrix.addValue(row, col, column[row]);
		}
	}
	return newMatrix;
};

// Move a set of columns from the old position to a new position.
// Assumes nothing is currently in the new position.
ms.matrix.prototype.moveColumns = function(oldColumn, newColumn, length) {
	if (!this.scatter) {
		ms.alert('Move columns only implemented on scatter.');
	}
	var newScatter = [this.scatter[0].slice(), this.scatter[1].slice(), this.scatter[2].slice()];
	var change = newColumn - oldColumn;
	// The columns are in scatter[1].
	for (var i = 0; i < newScatter[1].length; i++) {
		var s = newScatter[1][i];
		if (oldColumn <= s && s < oldColumn + length) {
			newScatter[1][i] += change;
		}
	}
	return new ms.matrix('scatter', newScatter);
};

// Invert the matrix..
ms.matrix.prototype.inv = function() {
	var result = numeric.inv(this.asFull())
	return new ms.matrix('full', result);
};

// Get the height.
ms.matrix.prototype.height = function() {
	return this.asFull().length;
};

// Get the x-value assuming this is a vector.
ms.matrix.prototype.x = function() {
	var row = this.asFull()[0];
	if (row.length != 1) {
		ms.alert('This matrix is not a vector.');
	}
	return row[0];
};

// Get the y-value assuming this is a vector.
ms.matrix.prototype.y = function() {
	var row = this.asFull()[1];
	if (row.length != 1) {
		ms.alert('This matrix is not a vector.');
	}
	return row[0];
};
