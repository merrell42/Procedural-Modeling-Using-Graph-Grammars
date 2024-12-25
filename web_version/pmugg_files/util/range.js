ms.range = function(low, high, opt_tileLength) {
	this.data = [low, high];
	this.tileLength = opt_tileLength || 0;
};

ms.range.prototype.intersect = function(rangeB) {
	return new ms.range(
		Math.max(this.data[0], rangeB.data[0]),
		Math.min(this.data[1], rangeB.data[1]),
		ms.range.lcm(this.tileLength, rangeB.tileLength));
};

ms.range.prototype.isEmpty = function() {
	return (this.data[1] < this.data[0]);
};

ms.range.prototype.sample = function() {
	if (this.tileLength == 0) {
		return ms.randomUniform(this.data[0], this.data[1]);
	} else {
		var low = Math.ceil(this.data[0] / this.tileLength);
		var high = Math.floor(this.data[1] / this.tileLength);
		
		return low + ms.random(high - low + 1);
	}
};

ms.range.errorMargin = 1e-5;

ms.range.prototype.isInside = function(x) {
	var m = ms.range.errorMargin;
	return (this.data[0] - m <= x) && (x <= this.data[1] + m);
};

// Transform range for x into range for a * x + b.
ms.range.prototype.transform = function(a, b) {
	if (a > 0) {
		return new ms.range(a * this.data[0] + b, a * this.data[1] + b, a * this.tileLength);
	} else {
		return new ms.range(a * this.data[1] + b, a * this.data[0] + b, a * this.tileLength);
	}
};

// Transform range for x into range for a * x + b.
ms.range.transformCreate = function(a, b, rangeB) {
	if (a == 0) {
		if (rangeB.isInside(b)) {
			return new ms.range(-Infinity, Infinity);
		} else {
			return new ms.range(Infinity, -Infinity);
		}
	}
	return rangeB.transform(1 / a, -b / a);
};

ms.range.prototype.print = function(a, b, rangeB) {
	console.log(this.data[0] + ', ' + this.data[1]);
};

ms.range.gcd = (...arr) => {
  const _gcd = (x, y) => ((y < 1e-3) ? x : ms.range.gcd(y, x % y));
  return [...arr].reduce((a, b) => _gcd(a, b));
};

// Least common multiple. But return the opposite if either a or b is 0.
ms.range.lcm = function(a, b) {
	if (a == 0) {
		return b;
	}
	if (b == 0) {
		return a;
	}
	return (a * b) / ms.range.gcd(a, b);   
};