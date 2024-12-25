ms.guideMutator = function(classifier, timer, notifyFunc) {
	this.classifier = classifier;
	this.timer = timer;
	this.notify = notifyFunc;
	this.tasks = [];
	this.status = ms.guideMutator.status.SUCCESS;
	this.breakTime = 0;
	this.earlyEndTime = 0;
	this.endTime = 0;
	this.seedCount = 0;
	this.pauseCount = -1;
	this.nodeStats = new ms.nodeStats();
	this.optimizer = new ms.optimizer(this.nodeStats);
	
	this.hierarchyMutator = ms.globalSettings.get('Use Network') ?
		new ms.networkMutator(classifier, this.nodeStats) :
		new ms.familyTreeMutator(classifier, this.nodeStats);

	// This is only for debugging. We include this as a watch statement so it gets redrawn.
	ms.globalMutator = this;
	ms.guideMutator.forceContinued = false;
};

ms.guideMutator.taskCount = 0;
ms.guideMutator.findMutableVertexAttempts = 20;
ms.guideMutator.findMutableLineAttempts = 20;
ms.guideMutator.forceContinued = false;

ms.guideMutator.prototype.seed = function(mutationArea) {
	this.hierarchyMutator.setMutationArea(mutationArea);
	this.seedCount = 0;
};

ms.guideMutator.forceContinue = function() {
	ms.guideMutator.forceContinued = true;
};

ms.guideMutator.closeInspection = function(taskCount) {
	if (ms.guideMutator.forceContinued) {
		return false;
	}
	var taskStop = ms.globalSettings.get('Task Stop');
	var taskStep = ms.globalSettings.get('Task Step');
	if (taskStop >= 0) {
		return taskCount >= taskStop && ((taskCount - taskStop) % taskStep == 0);
	} else {
		return ms.globalSettings.get('Debug Each Task');
	}
};

ms.guideMutator.prototype.mutate = function() {
	var probabilities = [1, 1, 10];
	var done = false;
	var self = this;
	while (!done) {
		switch(ms.randomDistribution(probabilities)) {
			case 0:
				ms.timerG.start('Add Fragment');
				var success = this.hierarchyMutator.addStartInstance();
				ms.timerG.stop('Add Fragment');
				if (success) {
					return;
				} else {
					probabilities[0] = 0;
				}

				break;
			case 1:
				ms.timerG.start('Remove Fragment');
				var success = this.hierarchyMutator.changeRandomInstance(true);
				ms.timerG.stop('Remove Fragment');
				if (success) {
					return;
				} else {
					probabilities[1] = 0;
				}
				break;
			case 2:
				// TODO: Do not allow fully connected edges to be destroyed.
				ms.timerG.start('Modify Fragment');
				var success = this.hierarchyMutator.changeRandomInstance(false);
				ms.timerG.stop('Modify Fragment');
				if (success) {
					return;
				} else {
					probabilities[2] = 0;
				}
				break;
		}
		// Uncomment this part to force there to be fewer starter transitions.
		// We cannot skip a starter transition on the first task.
		if (ms.globalSettings.get('Fewer Start Transitions') &&
			(ms.guideMutator.taskCount > 1)) {
			return;
		}

		// The change has already been rejected.
		if (this.nodeStats.costChange['reject'] > 0) {
			return;
		}

		done = true;
		for (var i = 0; i < probabilities.length; i++) {
			if (probabilities[i] > 0) {
				done = false;
			}
		}
	}
};

ms.guideMutator.prototype.resolve = function(outputModel) {
	if (this.status == ms.guideMutator.status.PAUSED) {
		return this.status;
	}
	ms.timerG.start('Guide Mutator');
	while (true) {
		var taskCount = ++ms.guideMutator.taskCount;
		if (ms.guideMutator.closeInspection(taskCount)) {
			this.displayStats();
			debugger;
		}
		this.mutate();

		var cost = this.optimizer.computeCost();
		ms.timerG.start('Accept Or Reject');
		if (this.optimizer.isAccepted(cost)) {
			this.accept();
		} else {
			this.reject();
		}
		ms.timerG.stop('Accept Or Reject');
		ms.timerG.start('Save');
		this.nodeStats.save();
		ms.timerG.stop('Save');

		var time = Date.now();
		var maxTimeEnabled = ms.globalSettings.get('Max Time Enabled');
		var maxIterations = ms.globalSettings.get('Max Iterations');
		if ((maxTimeEnabled && (time > this.endTime || (time > this.earlyEndTime) && (this.nodeStats.getBadVertices().length == 0))) ||
			(taskCount >= maxIterations)){
			ms.timerG.stop('Guide Mutator');
			this.status = ms.guideMutator.status.FINISHED;
			return this.status;
		}
		if (taskCount == this.pauseCount) {
			this.pause();
			return this.status;
		}
		if (time > this.breakTime) {
			ms.timerG.stop('Guide Mutator');
			return ms.guideMutator.status.UNRESOLVED;
		}
	}
};

ms.guideMutator.prototype.accept = function() {
	if (this.nodeStats.getBadVertices().length == 0) {		
		this.seedCount++;
	}
	var taskCount = ms.guideMutator.taskCount;
	if (ms.guideMutator.closeInspection(taskCount)) {
		window.console.log(taskCount + ' accepted.');
	}
};

ms.guideMutator.prototype.reject = function() {
	var taskCount = ms.guideMutator.taskCount;
	this.nodeStats.restore();
	if (ms.guideMutator.closeInspection(taskCount) || ms.globalSettings.get('Debug Alerts')) {
		this.optimizer.verifyCost();
	}
	if (ms.guideMutator.closeInspection(taskCount)) {
		window.console.log(taskCount + ' rejected.');
	}
};

ms.guideMutator.prototype.findMutableVertex = function() {
	var vertexNodes = Object.values(this.nodeStats.nodes.vertex);
	if (vertexNodes.length > 0) {
		for (var i = 0; i < ms.guideMutator.findMutableVertexAttempts; i++) {
			var vertex = ms.pick(vertexNodes).getElement();
			if (vertex.isMoveable()) {
				return vertex;
			}
		}
	}
	return null;
};

ms.guideMutator.prototype.save = function(model) {
	var cost = this.optimizer.computeCost();
	this.optimizer.accept(cost);
	this.nodeStats.save();
};
	
ms.guideMutator.prototype.initializeModel = function(model) {
	this.edgeTypeStarted = [];
	this.pauseCount = -1;
	model.setNodeStats(this.nodeStats);

	var boundaryGroups = this.classifier.getBoundaryGroups();
	if (boundaryGroups && boundaryGroups.length > 0) {
		var ringInstance = new ms.ringInstance(this.classifier.getEmptyRing(), this.nodeStats);
		var ringGroup = ms.ringGroup.createFromRing(ringInstance.getRing());
		var startInstance = new ms.ringGroupInstance(ringGroup);
		startInstance.addRingInstance(0, ringInstance);
		var transition = {startInstance: startInstance, endGroup: ms.pick(boundaryGroups), initialBoundary: true};
		return this.hierarchyMutator.applyTransition(transition);
	} else {
		return true;
	}
};

ms.guideMutator.status = {
	FAILED: 0,
	UNRESOLVED: 1,
	PAUSED: 2,
	SUCCESS: 3,
	FINISHED: 4,
};

ms.guideMutator.prototype.getStatus = function() {
	return this.status;
};

ms.guideMutator.prototype.pause = function() {	
	this.displayStats();
	this.notify(true);
	this.status = ms.guideMutator.status.PAUSED;
};

ms.guideMutator.prototype.unresolve = function() {
	this.status = ms.guideMutator.status.UNRESOLVED;
};

ms.guideMutator.prototype.step = function(numTasks) {
	this.displayStats();
	this.pauseCount = ms.guideMutator.taskCount + numTasks;
};

ms.guideMutator.prototype.setBreakTime = function(breakTime) {
	this.breakTime = breakTime;
};

ms.guideMutator.prototype.setEndTime = function(earlyEndTime, endTime) {
	this.earlyEndTime = earlyEndTime;
	this.endTime = endTime;
};

ms.guideMutator.prototype.reset = function() {
	this.status = ms.guideMutator.status.UNRESOLVED;
};

ms.guideMutator.hideStats = function() {
	document.getElementById('statistics').innerHTML = '';
};

ms.guideMutator.prototype.displayStats = function() {
	var taskCount = ms.guideMutator.taskCount;
	var stats = 'Iteration: ' + taskCount + ' / ' + ms.globalSettings.get('Max Iterations');
	if (!ms.mvp) {
		stats += ' ' + this.optimizer.getPrevCost().sum.toFixed(1);
	}
	document.getElementById('statistics').innerHTML = stats;
};

ms.guideMutator.prototype.report = function() {
	var cost = this.optimizer.getPrevCost();
	var summary = cost.sum + ' ' + (this.timer.getTiming('everything') / 1000) + 's ';
	summary += (ms.guideMutator.taskCount / this.timer.getTiming('everything')).toFixed(4);
	var details = JSON.stringify(cost) + '\n';
	details += this.timer.reportMean();
	console.log('# of vertices: ' + this.nodeStats.getCount('vertex'));
	return {summary: summary, details: details};
};