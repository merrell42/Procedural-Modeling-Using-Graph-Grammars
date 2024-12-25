ms.fastMath = {};

ms.fastMath.mat = function(data) {
	this.data = data;
	this._size = [data.length, data.length > 0 ? data[0].length : 0];
};

ms.fastMath.mat.prototype.size = function() {
	return this._size;
};

ms.fastMath.mat.prototype.set = function(index, value) {
	this.data[index[0]][index[1]] = value;
};

ms.fastMath.mat.prototype.get = function(index, value) {
	return this.data[index[0]][index[1]];
};

ms.fastMath.mat.prototype.valueOf = function() {
	return this.data;
};

ms.fastMath.mat.prototype.subset = function(indices, opt_replacement) {
	return ms.fastMath.subset(this, indices, opt_replacement);
};

ms.fastMath.matrix = function(data) {
	return new ms.fastMath.mat(data);
};

ms.fastMath.zeros = function(size0, size1) {
	var row = new Array(size1).fill(0);
	var rows = [];
	for (var i = 0; i < size0; i++) {
		rows.push(row.slice());
	}
	return new ms.fastMath.mat(rows);
};

// Assumes the matrices are the same size.
ms.fastMath.add = function(A, B) {
	var result = ms.fastMath.zeros(A._size[0], A._size[1]);
	var a = A.data;
	var b = B.data;
	for (var i = 0; i < A._size[0]; i++) {
		for (var j = 0; j < A._size[1]; j++) {
			result.data[i][j] = a[i][j] + b[i][j];
		}
	}
	return result;
};

// Assumes the matrices are the same size.
ms.fastMath.subtract = function(A, B) {
	var result = ms.fastMath.zeros(A._size[0], A._size[1]);
	var a = A.data;
	var b = B.data;
	for (var i = 0; i < A._size[0]; i++) {
		for (var j = 0; j < A._size[1]; j++) {
			result.data[i][j] = a[i][j] - b[i][j];
		}
	}
	return result;
};

ms.fastMath.det2x2 = function(a, b, c, d) {
	return a * d - b * c;
};

ms.fastMath.det = function(A) {
	if (A._size[0] == 2 && A._size[1] == 2) {
		var a = A.data;
		return ms.fastMath.det2x2(a[0][0], a[0][1], a[1][0], a[1][1]);
	} else if (A._size[0] == 3 && A._size[1] == 3) {
		var data = A.data;
		var a = data[0][0];  var b = data[0][1];  var c = data[0][2];
		var d = data[1][0];  var e = data[1][1];  var f = data[1][2];
		var g = data[2][0];  var h = data[2][1];  var i = data[2][2];
		return a*e*i + b*f*g + c*d*h - c*e*g - b*d*i - a*f*h;
	} else {
		ms.alert('det only implemented for 2x2 and 3x3.');
	}
};

ms.fastMath.inv = function(A) {
	if (A._size[0] == 2 && A._size[1] == 2) {
		var det1 = 1.0 / ms.fastMath.det(A);
		var a = A.data;
		var result = [[det1 * a[1][1], -det1 * a[0][1]], [-det1 * a[1][0], det1 * a[0][0]]];
		return ms.fastMath.matrix(result);
	} else if (A._size[0] == 3 && A._size[1] == 3) {
		var det1 = 1.0 / ms.fastMath.det(A);
		var data = A.data;
		var result = [];
		for (var i = 0; i < 3; i++) {
			var row = [];
			for (var j = 0; j < 3; j++) {
				var s = (j + 1) % 3;
				var t = (j + 2) % 3;
				var u = (i + 1) % 3;
				var v = (i + 2) % 3;
				var a = data[s][u];
				var b = data[t][u];
				var c = data[s][v];
				var d = data[t][v];
				row.push(det1 * ms.fastMath.det2x2(a, b, c, d));
			}
			result.push(row);
		}
		return ms.fastMath.matrix(result);
	} else {
		ms.alert('inv only implemented for 2x2 and 3x3.');
	}
};

// Assume these are both column vectors.
ms.fastMath.dot = function(A, B) {
	var a = A.data;
	var b = B.data;
	var sum = 0;
	for (var i = 0; i < A._size[0]; i++) {
		sum += a[i][0] * b[i][0];
	}
	return sum;
};

ms.fastMath.transpose = function(A) {
	var result = ms.fastMath.zeros(A._size[1], A._size[0]);
	var a = A.data;
	for (var i = 0; i < A._size[0]; i++) {
		for (var j = 0; j < A._size[1]; j++) {
			result.data[j][i] = a[i][j];
		}
	}
	return result;
};

ms.fastMath.multiply = function(A, B) {
	if (typeof B == 'number') {
		var result = ms.fastMath.zeros(A._size[0], A._size[1]);
		var a = A.data;
		var r = result.data;
		for (var i = 0; i < A._size[0]; i++) {
			for (var j = 0; j < A._size[1]; j++) {
				r[i][j] = a[i][j] * B;
			}
		}
		return result;
	} else {
		var result = ms.fastMath.zeros(A._size[0], B._size[1]);
		var a = A.data;
		var b = B.data;
		var r = result.data
		for (var i = 0; i < A._size[0]; i++) {
			for (var j = 0; j < B._size[1]; j++) {
				var cij = 0;
				for (var k = 0; k < A._size[1]; k++) {
					cij += a[i][k] * b[k][j];
				}
				r[i][j] = cij;
			}
		}
		return result;
	}
};

ms.fastMath.index = function(index0, index1) {
	return [index0, index1];
};

ms.fastMath.range = function(lower, upper) {
	var result = [];
	for (var i = lower; i < upper; i++) {
		result.push(i);
	}
	return result;
};

ms.fastMath.subset = function(A, indices, opt_replacement) {
	var indices0 = indices[0];
	var indices1 = indices[1];
	var a = A.data;
	if (opt_replacement) {
		var replacement = opt_replacement.data;
		for (var i = 0; i < indices0.length; i++) {
			var index0 = indices0[i];
			for (var j = 0; j < indices1.length; j++) {
				var index1 = indices1[j];
				a[index0][index1] = replacement[i][j];
			}
		}
		return A;
	} else {
		var result = mathG.zeros(indices0.length, indices1.length);
		var r = result.data;
		for (var i = 0; i < indices0.length; i++) {
			var index0 = indices0[i];
			for (var j = 0; j < indices1.length; j++) {
				var index1 = indices1[j];
				r[i][j] = a[index0][index1];
			}
		}
		return result;
	}
};

// This changes A.
ms.fastMath.concat = function(A, B) {
	A._size[1] += B._size[1];
	var size0 = A._size[0];
	var bSize1 = B._size[1];
	var a = A.data;
	var b = B.data;
	for (var i = 0; i < size0; i++) {
		for (var j = 0; j < bSize1; j++) {
			a[i].push(b[i][j]);
		}
	}
	return A;
};

ms.fastMath.test = function() {
	var A = [[0.5111387281709159, -0.2797539798576054], [0.351428224100073, 0.7387129829558294]];
	var B = [[0.9243097344671738, 0.8126861460327792], [0.20270884757161012, 0.4439440454855923]];
	var A0 = math.matrix(A);
	var B0 = math.matrix(B);
	var A1 = ms.fastMath.matrix(A);
	var B1 = ms.fastMath.matrix(B);
	
	var compare = function(result0, result1) {
		console.log(math.subtract(result0, math.matrix(result1.data))._data);
	}
	compare(
		math.add(A0, B0),
		ms.fastMath.add(A1, B1)
	);
	compare(
		math.subtract(A0, B0),
		ms.fastMath.subtract(A1, B1)
	);
	compare(
		math.inv(A0),
		ms.fastMath.inv(A1)
	);
	compare(
		math.multiply(A0, B0),
		ms.fastMath.multiply(A1, B1)
	);
}
