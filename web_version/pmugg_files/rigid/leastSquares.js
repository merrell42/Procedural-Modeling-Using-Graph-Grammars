ms.leastSquares = function() {};

ms.leastSquares.leastSquares = function(A, W, x) {
	var At = A.T();	
	var AtWx = At.dot(W).dot(x);
	var AtWA = At.dot(W).dot(A);
	return AtWA.solve(AtWx);
};

ms.leastSquares.findCost = function(A, W, x, c) {
	var error = A.dot(c).minus(x);
	var zero = !error.asFull().some(function(e) { return e[0] != 0; })
	if (zero) {
		return 0;
	}
	var cost = error.T().dot(W).dot(error);
	return cost.asFull()[0][0];	
};

/**
 * Compute least squares for all the points and just take out the part we need.
 * We may be able to assume that this is just the position of one other vertex
 * this is connected to plus the offset. That would be even faster.
 */ 
ms.leastSquares.findMu = function(A0, W, B, x, rowIndex) {
	var c = ms.leastSquares.leastSquares(A0, W, x);
	return c.asFull().splice(rowIndex, 2);
};

// Same as findMu just for the test function.
ms.leastSquares.findMuFast = function(A0, A, W, B, x) {
	var c = ms.leastSquares.leastSquares(A0, W, x);
	var mean = c.asFull().splice(2, 2);
	return new ms.matrix('full', mean);
};

// This is slower. I thought there might be a way to compute things faster doing
// something like this. sigma and rt almost cancel out most of their terms. They
// would if B was invertible.
ms.leastSquares.findMuSlow = function(A0, A, W, B, x) {
	var At = A.T();	
	var AtWA = At.dot(W).dot(A);
	var iAtWA = AtWA.inv();
	var S = A.dot(iAtWA).dot(At).dot(W);
	var I = ms.matrix.identity(A.height());
	var si = S.minus(I);

	var rt = B.T().dot(si.T()).dot(W).dot(si).dot(x);
	var sigma = ms.leastSquares.findSigma(A, W, B);
	return sigma.inv().dot(rt);
};

// Although this is conceptually simpler. It is slower because of the
// matrix inverse.
ms.leastSquares.findSigmaSlow = function(A, W, B) {
	var At = A.T();	
	var AtWA = At.dot(W).dot(A);
	var iAtWA = AtWA.inv();
	var S = A.dot(iAtWA).dot(At).dot(W);
	var I = ms.matrix.identity(A.height());
	var si = S.minus(I);
	return B.T().dot(si.T()).dot(W).dot(si).dot(B);
};

// A faster version that does not directly compute the inverse.
ms.leastSquares.findSigma = function(A, W, B) {
	var Bt = B.T();
	var BtW = Bt.dot(W);
	var s1 = BtW.dot(B);
	if (A.isEmpty()) {
		return s1;
	}

	var At = A.T();
	var AtW = At.dot(W);
	var B1 = B.copy();
	var B2 = B1.spliceColumns(1, 1);
	var AtWA = AtW.dot(A);
	var BtWA = BtW.dot(A);
	var p1 = AtWA.solve(AtW.dot(B1));
	var p2 = AtWA.solve(AtW.dot(B2));
	if (p1.isEmpty() || p2.isEmpty()) {
		return s1;
	}
	var q1 = BtWA.dot(p1);
	var q2 = BtWA.dot(p2);
	var s2 = ms.matrix.combineColumns([q1, q2]);
	
	return s1.minus(s2);
};

ms.leastSquares.testAccuracy = function(findSigma) {
	var trials = 100;
	var errorSum = 0;
	for (var i = 0; i < 100; i++) {
		var A0 = ms.matrix.random(20, 6, 10);
		var W = ms.matrix.randomDiagonal(20);
		var x = ms.matrix.random(6, 1, 10);
		var c = ms.leastSquares.leastSquares(A0, W, x);
		var cost1 = ms.leastSquares.findCost(A0, W, x, c);
	
		var A = A0.copy();
		var B = A.spliceColumns(2, 2);
		var sigma = findSigma(A, W, B);
		
		var c0 = c.asFull().slice();
		var mean = c0.splice(2, 2);
		d0 = new ms.matrix('full', mean);
		
		var deltaD = ms.matrix.random(2, 1, 10);
		var d = d0.add(deltaD);
		var xd = x.minus(B.dot(d));
		var c = ms.leastSquares.leastSquares(A, W, xd);
		var cost2 = ms.leastSquares.findCost(A, W, xd, c);
		var actual = cost2 - cost1;
		
		var deltaCost = deltaD.T().dot(sigma).dot(deltaD);
		var predicted = deltaCost.asFull()[0][0];
		errorSum = Math.abs(actual - predicted);
	}
	window.console.log(errorSum / trials);
};

ms.leastSquares.testSpeed = function(constraints, vertices, trials, findSigma) {
	var startTime = Date.now();
	for (var i = 0; i < trials; i++) {
		var A0 = ms.matrix.random(constraints, vertices, 10);
		var W = ms.matrix.randomDiagonal(constraints);
		var x = ms.matrix.random(vertices, 1, 10);
		var A = A0.copy();
		var B = A.spliceColumns(2, 2);
		var sigma = findSigma(A, W, B);
	}
	var endTime = Date.now();
	return endTime - startTime;
};

ms.leastSquares.testSpeedMu = function(constraints, vertices, trials, findMu) {
	var startTime = Date.now();
	for (var i = 0; i < trials; i++) {
		var A0 = ms.matrix.random(constraints, vertices, 10);
		var W = ms.matrix.randomDiagonal(constraints);
		var x = ms.matrix.random(vertices, 1, 10);
		var A = A0.copy();
		var B = A.spliceColumns(2, 2);
		var mu = findMu(A0, A, W, B, x);
	}
	var endTime = Date.now();
	return endTime - startTime;
};

ms.leastSquares.test = function(findSigma) {
	ms.leastSquares.testAccuracy(ms.leastSquares.findSigmaSlow);
	ms.leastSquares.testAccuracy(ms.leastSquares.findSigma);

	window.console.log(ms.leastSquares.testSpeedMu(20, 6, 500, ms.leastSquares.findMuSlow));
	window.console.log(ms.leastSquares.testSpeedMu(20, 6, 500, ms.leastSquares.findMuFast));
	window.console.log(ms.leastSquares.testSpeedMu(200, 6, 10, ms.leastSquares.findMuSlow));
	window.console.log(ms.leastSquares.testSpeedMu(200, 6, 10, ms.leastSquares.findMuFast));

  window.console.log('Small');
	window.console.log(ms.leastSquares.testSpeed(20, 6, 500, ms.leastSquares.findSigmaSlow));
	window.console.log(ms.leastSquares.testSpeed(20, 6, 500, ms.leastSquares.findSigma));
	window.console.log(ms.leastSquares.testSpeed(20, 6, 500, function() {}));

	window.console.log('Large');
	window.console.log(ms.leastSquares.testSpeed(200, 60, 10, ms.leastSquares.findSigmaSlow));
	window.console.log(ms.leastSquares.testSpeed(200, 60, 10, ms.leastSquares.findSigma));
	window.console.log(ms.leastSquares.testSpeed(200, 60, 10, function() {}));
};