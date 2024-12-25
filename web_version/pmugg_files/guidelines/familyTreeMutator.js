ms.familyTreeMutator = function(classifier, nodeStats) {
	this.classifier = classifier;
	this.ringTree = classifier.getFamilyTree().getRingTree();
	this.nodeStats = nodeStats;
	this.edgeTypeStarted = [];
	this.mutationArea = null;
};

ms.familyTreeMutator.prototype.setMutationArea = function(mutationArea) {
	this.mutationArea = mutationArea;
};

ms.familyTreeMutator.prototype.addEdgeTypeStarted = function(id) {
	if (!this.edgeTypeStarted.includes(id)) {
		this.edgeTypeStarted.push(id);
	}
};

// We randomly choose several lines, but then choose between them based on their length.
ms.familyTreeMutator.linesToChoose = 10;
ms.familyTreeMutator.findRingAttempts = 5;
ms.familyTreeMutator.effortLimit = 5;

ms.familyTreeMutator.prototype.addStartInstance = function() {
	var starterRing = this.classifier.getStarterRing();
	var transition = null;
	if (starterRing) {
		var instance = new ms.ringInstance(starterRing, this.nodeStats);
		transition = this.changeInstance(instance);
	}
	if (transition) {
		var connectedTypes = transition.endGroup.getFullyConnectedTypes();
		var self = this;
		connectedTypes.forEach(function(id) {
			self.addEdgeTypeStarted(id);
		});
	}
	return !!transition;
};

ms.familyTreeMutator.prototype.changeRandomInstance = function(justDestructible) {
	var lines = this.nodeStats.getElements('line');
	if (lines.length == 0) {
		return false;
	}
	var ringTree = this.ringTree[justDestructible ? 'destroy' : 'change'];
	for (var i = 0; i < ms.familyTreeMutator.findRingAttempts; i++) {
		var linesToChoose = [];
		var weights = [];
		for (var j = 0; j < ms.familyTreeMutator.linesToChoose; j++) {
			var line = ms.pick(lines);
			linesToChoose.push(line);
			weights.push(line.getSegment().getLength());
		}
		
		var line = linesToChoose[ms.pickByWeight(weights)];
		ms.timerG.start('Match Morphism');
		var instances = ringTree.getRingInstances(line);
		ms.timerG.stop('Match Morphism');
		if (instances.length > 0) {			
			var instance = ms.pick(instances);
			return this.changeInstance(instance);
		}
	}
	return false;
};

ms.familyTreeMutator.prototype.changeInstance = function(instance) {
	var transition = instance.randomTransitionGroups(this.edgeTypeStarted);
	if (!transition) {
		return null;
	}
	var ignoredIfWindingDisabled = 
		transition.startInstance.group.ignoredIfWindingDisabled() ||
		transition.endGroup.ignoredIfWindingDisabled();
	
	var success = this.applyTransition(transition);
	if (success) {
		if (ignoredIfWindingDisabled) {
			ms.alert('ignoredIfWindingDisabled transition successful.');
		}
		return transition;
	} else {
		return null;
	}
};

ms.familyTreeMutator.prototype.applyTransition = function(transition) {	
	var found = transition.startInstance.findMissingRings(transition.endGroup, this.mutationArea, this.ringTree);
	if (!found) {
		/* var startGroup = transition.startInstance.group;
		if (startGroup.rings.length == 2 && startGroup.rings[0] == startGroup.rings[1] && startGroup.rings[0].getNumConnectors() == 2) {
			ms.alert('There could be an issue with trying to use the same line in two instances.');
		} */
		return false;
	}
	ms.timerG.start('Build Normally');
	var transistor = ms.transistor.buildNormally(transition);
	ms.timerG.stop('Build Normally');
	if (!transistor) {
		this.nodeStats.costChange['reject'] = 1e6;
		return false;
	}
	ms.taskDebug();
	var effort = 0;
	while (effort < ms.familyTreeMutator.effortLimit) {
		ms.timerG.start('Transistor Solve');
		var success = transistor.solve(this.mutationArea);
		ms.timerG.stop('Transistor Solve');
		if (success) {
			return true;;
		}
		transistor.freeVertex();
		effort++;
	}
	transistor.reject();
	return false;
};