ms.optimizer = function(nodeStats) {
	this.nodeStats = nodeStats;
	this.prevCost = this.computeCost();
};

ms.optimizer.detailedCost = false;
ms.optimizer.costScale = 0.01;

ms.optimizer.prototype.computeCost = function() {
	var cost = {};

	// Compute line distance from the change.
	var prevLogLineDistances = this.prevCost ? this.prevCost.logLineDistances : 0;	
	cost.logLineDistances = prevLogLineDistances + this.nodeStats.costChange.lineDistance;
	cost.reject = this.nodeStats.costChange.reject || 0;
	var numLines = this.nodeStats.count.line;
	var desiredLines = ms.globalSettings.get('Desired Lines');
	var desiredCost  = ms.globalSettings.get('Desired Lines Cost');
	cost.desiredLines = desiredCost * Math.max((desiredLines - numLines) / desiredLines, -1);
	
	var desiredVertexWeight  = ms.globalSettings.get('Desired Vertex Weight');
	var desirabilitySum = 0;
	this.nodeStats.getElements('vertex').forEach(function(vertex) {
		var decoration = vertex.getState().getType().getDecoration();
		if (decoration) {
			desirabilitySum -= decoration.get('Desirability');
		}
	});
	cost.desiredVertex = desiredVertexWeight * desirabilitySum;
	
	cost.sum = ms.optimizer.costScale *	(cost.logLineDistances + cost.reject + cost.desiredLines + cost.desiredVertex);
	return cost;
};

ms.optimizer.prototype.accept = function(cost) {
	this.prevCost = cost;
};

// Accept or reject the mutation based on the Metropolis algorithm.
ms.optimizer.prototype.isAccepted = function(cost) {
	var difference = this.prevCost.sum - cost.sum;
	var prob = Math.exp(ms.globalSettings.get('Beta') * difference);
	if (Math.random() < prob) {
		this.prevCost = cost;
		return true;
	} else {
		return false;
	}
};

ms.optimizer.prototype.getPrevCost = function() {
	return this.prevCost;
};

// Verify that the cost is restored to its previous value when the model is restored.
ms.optimizer.prototype.verifyCost = function() {
	var costChange = this.computeCost().sum - this.prevCost.sum;
	if (Math.abs(costChange) > 1e-10) {
		ms.alert('Model was not restored correctly.');
		debugger;
	}
};
