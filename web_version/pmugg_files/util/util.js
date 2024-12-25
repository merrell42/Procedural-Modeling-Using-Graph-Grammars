ms.util = function() {};

ms.util.fixAngle = function(angle) {
	while (angle > Math.PI) {
		angle -= 2 * Math.PI;
	}
	while (angle <= -Math.PI) {
		angle += 2 * Math.PI;
	}
	return angle;
};

ms.util.angleDifference = function(a, b) {
	return ms.util.fixAngle(a - b);
};

ms.util.EPS = 1e-5;

// Returns 1, if the angle turns counter-clockwise.
// Returns -1, if the angle turns clockwise.
// Returns 0, otherwise.
ms.util.angleTurn = function(prev, next) {
	if (prev > Math.PI - ms.util.EPS) { prev -= 2 * Math.PI };
	if (next > Math.PI - ms.util.EPS) { next -= 2 * Math.PI };
	
	if (Math.abs(prev - next) <= Math.PI) {
		return 0;
	} else {
		if (prev >= 0 && next < 0) {
			return 1;
		} else if (next >= 0 && prev < 0) {
			return -1;
		}
	}
};

ms.util.angleWedges = function(prev, next) {
	if (prev >= Math.PI) { prev -= 2 * Math.PI };
	if (next >= Math.PI) { next -= 2 * Math.PI };
	
	var sign = 0;
	if (prev >= 0 && next < 0) {
		sign = 1;
	} else if (next >= 0 && prev < 0) {
		sign = -1;
	}

	// The 0.01 sign makes positive turns more common when
	// angles are in opposite directions.
	if (Math.abs(prev - next) + 0.01 * sign > Math.PI) {
		return sign;
	} else {
		return 0;
	}
};

ms.util.wedgeTurns = function(angles) {
	var turns = 0;
	for (var i = 0; i < angles.length - 1; i++) {
		turns += ms.util.angleWedges(angles[i], angles[i + 1]);
	}
	return turns;
};

ms.util.fixAngleWedges = function(angle) {
	if (angle >= Math.PI) { angle -= 2 * Math.PI };
	if (angle < -Math.PI) { angle += 2 * Math.PI };
	return angle;
};

ms.remove = function(e, array) {
	var index = array.indexOf(e);
	if (index != -1) {
		array.splice(index, 1);
	} else {
		ms.alert('Removing something that cannot be found.');
	}
};

ms.maybeRemove = function(e, array) {
	var index = array.indexOf(e);
	if (index != -1) {
		array.splice(index, 1);
	}
};

ms.addToObject = function(obj, prop, value) {
	if (!obj.hasOwnProperty(prop)) {
		obj[prop] = [];
	}
	obj[prop].push(value);
};

// Add to an array at the given indices.
ms.addToArray = function(array, indices, value) {
	var index = indices[0];	
	if (!array[index]) {
		array[index] = [];
	}
	if (indices.length > 1) {
		ms.addToArray(array[index], indices.splice(1), value);
	} else {
		array[index].push(value);
	}
};

// Add elements from b to a. If they do not already exist in a.
ms.union = function(a, b) {
	b.forEach((elemB) => {
		if (!a.includes(elemB)) {
			a.push(elemB);
		}
	});
};

ms.taskDebug = function() {
	if (ms.guideMutator.closeInspection(ms.guideMutator.taskCount)) {
		debugger;
	}
};

ms.alert = function(message) {
	message = ms.globalSettings.get('Seed') + ' ' + message;
	window.console.log(message);
	if (ms.globalSettings.get('Debug Alerts')) {
		debugger;
	}
};

ms.assert = function(statement, errorMessage) {
	if (!statement) {
		debugger;
		alert(errorMessage);
	}
};

ms.nullFunction = function() {};

ms.pick = function(array) {
  var index = ms.random(array.length);
	return array[index];
};

ms.pickOne = function(array) {
  if (array.length != 1) {
		ms.alert('Wrong number of elements in the array.');
	}
	return ms.pick(array);
};

ms.pickByWeight = function(weights) {
	var sum = 0;
	cumulativeSums = [];
	for (var i = 0; i < weights.length; i++) {
		sum += weights[i];
		cumulativeSums.push(sum);
	}
	var randomValue = sum * Math.random();
	for (var i = 0; i < weights.length; i++) {
		if (randomValue < cumulativeSums[i]) {
			return i;
		}
	}
};

ms.random = function(n) {
	return Math.floor(Math.random() * n);
};

// Like random but produces the same result each time from the seed.
ms.util.consistentRandom = function(n, seed) {
	// r is a random number between 0 and 1.
	var x = Math.sin(seed) * (10000 + seed);
	var r = x - Math.floor(x);
	return Math.floor(r * n);
};

ms.randomDistribution = function(probabilityMass) {
	var sums = [];
	var sum = 0;
	for (var i = 0; i < probabilityMass.length; i++) {
		sum += probabilityMass[i];
		sums.push(sum);
	}
	if (sum == 0) {
		return -1;
	}
	var r = sum * Math.random();
	var index = 0;
	while (r > sums[index]) {
		index++;
	}
	return index;
};

// Generate a random variable that is a Gaussian mean zero, standard deviation 1.
// Uses the Box-Muller transformation.
ms.randomGaussian = function() {
	return Math.sqrt(-2 * Math.log(Math.random())) * Math.cos(2 * Math.PI * Math.random());
};

ms.randomUniform = function(lower, upper) {
	if (lower > upper) {
		ms.alert('Lower bound is greater than upper bound.');
	}
	return Math.random() * (upper - lower) + lower;
};

ms.util.originalRandom = Math.random;

ms.util.random = function(seed, count) {
	var x = Math.sin(seed + count) * (10000 + seed);
	return x - Math.floor(x);
};

ms.util.updateRandomMode = function() {
	var randomMode = ms.globalSettings.get('New Seed') ? 1 : 2;
	switch (randomMode) {
		case 0:
			// This is never used, but we may want to include the option of sticking with
			// the normal random number generator.
			Math.random = ms.util.originalRandom;
			break;
		case 1:			
			ms.globalSettings.set('Seed', Math.floor(ms.globalSettings.get('Seed') + 1));
		case 2:
			ms.randomCount = 0;
			Math.random = function() {
				ms.randomCount++;
				var seed = ms.globalSettings.get('Seed');
				return ms.util.random(seed, ms.randomCount);
			};
			break;
	}
};

ms.util.removeDuplicates = function(arrayA) {
	var result = [];
	arrayA.forEach(function(a) {
		if (!result.includes(a)) {
			result.push(a);
		}
	});
	return result;
};

ms.util.arraysEquivalent = function(arrayA, arrayB) {
	for (var i = 0; i < arrayA.length; i++) {
		if (!arrayB.includes(arrayA[i])) {
			return false;
		}
	}
	for (var i = 0; i < arrayB.length; i++) {
		if (!arrayA.includes(arrayB[i])) {
			return false;
		}
	}
	return true;
};

ms.util.arraysEqual = function(arrayA, arrayB) {
	if (arrayA.length != arrayB.length) {
		return false;
	}
	for (var i = 0; i < arrayA.length; i++) {
		if (arrayA[i] != arrayB[i]) {
			return false;
		}
	}
	return true;
};

ms.util.last = function(array) {
	return array[array.length - 1];
};

ms.util.clamp = function(lower, upper, x) {
	return Math.max(lower, Math.min(upper, x));
};

// Sequence of numbers from a to b.
ms.util.sequence = function(a, b) {
	var result = [];
	for (var i = a; i <= b; i++) {
		result.push(i);
	}
	return result;
};

ms.util.fastConcat = function(allData, newData) {
	for (var i = 0; i < newData.length; i++) {
		allData.push(newData[i]);
	}
};

ms.util.maxDim = function(n) {
	var coords = [[Math.abs(n.x), 0], [Math.abs(n.y), 1], [Math.abs(n.z), 2]];
	return coords.sort(function(a, b) { return b[0] - a[0]; })[0][1];
};

ms.continue = function() {
	ms.guideMutator.forceContinue();
};

ms.pause = function() {
	window.controller.currentController.synthesizer.pause();
};

ms.resume = function() {
	window.controller.currentController.synthesizer.resume();
};
