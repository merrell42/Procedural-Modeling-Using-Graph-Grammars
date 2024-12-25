ms.networkMutator = function(classifier, nodeStats) {
	this.hierarchy = classifier.hierarchy;
	this.is3D = classifier.is3D;
	this.nodeStats = nodeStats;
	this.edgeTypeStarted = [];
	this.mutationArea = null;
	var groundEnabled = classifier.hierarchy.groundTransitions.length > 0;
	this.mapFinder = new ms.netGraphMapFinder(nodeStats, groundEnabled);
};

ms.networkMutator.prototype.setMutationArea = function(mutationArea) {
	this.mutationArea = mutationArea;
};

// We randomly choose several lines, but then choose between them based on their length.
ms.networkMutator.linesToChoose = 10;

ms.networkMutator.prototype.addStartInstance = function() {
	var transition = this.hierarchy.getStarterTransition();
	return this.applyTransition(transition);
};

ms.networkMutator.prototype.changeRandomInstance = function(justDestructible) {
	var transition = justDestructible ?
		this.hierarchy.getRemoveTransition() :
		this.hierarchy.getTransition();
	return this.applyTransition(transition);
};

ms.networkMutator.prototype.applyTransition = function(transition, findMap) {
	ms.timerG.start('Find Transition Map');
	var netGraphMap = transition && this.mapFinder.findMap(transition.startNet);
	ms.timerG.stop('Find Transition Map');
	if (!netGraphMap) {
		return false;
	}
	// if (transition.startNet.getConnectors().length == 4) {
	// if (ms.guideMutator.taskCount == 28) {
	//	debugger;
	// }
	transition.map = netGraphMap;

	ms.timerG.start('Build Normally');
	var transistor = ms.netTransistor.buildNormally(transition, this.nodeStats, this.is3D);
	ms.timerG.stop('Build Normally');
	if (!transistor) {
		this.nodeStats.costChange['reject'] = 1e6;
		return false;
	}
	ms.taskDebug();
	var effort = 0;
	while (effort < ms.globalSettings.get('Mutator Effort Limit')) {
		ms.timerG.start('Transistor Solve');
		var success = transistor.solve(this.mutationArea);
		ms.timerG.stop('Transistor Solve');
		if (success) {
			return true;
		}
		for (var i = 0; i < ms.globalSettings.get('Vertices to Free'); i++) {
			transistor.freeVertex();
		}
		effort++;
	}
	transistor.reject();
	return false;
};