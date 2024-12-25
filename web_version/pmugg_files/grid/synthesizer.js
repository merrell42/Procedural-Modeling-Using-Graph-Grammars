ms.synthesizer = function(example, view, cameraController) {
	this.seeder = null;
	this.observer = view;
	this.model = new ms.model([10, 10, 1]);
	this.mutator = null;
	this.mutationArea = null;
	this.numAttempts = 0;
	this.timer = ms.timerG;
	this.cameraController = cameraController;
	this.is3D = false;
	this.callback = function() {};

	this.mutationOrigin = [0, 0, 0];
};

ms.synthesizer.BORDER_THRESHOLD = 0.1;

ms.synthesizer.prototype.getModel = function() {
	return this.model;
};

ms.synthesizer.prototype.offset = function() {
	return ms.vec2.ORIGIN;
};

ms.synthesizer.prototype.notify = function(opt_fullRedraw) {
	ms.timerG.start('Redraw');
	this.observer.redraw(this.getModel(), opt_fullRedraw || false);
	this.mutator.displayStats();
	this.mutator.setBreakTime(Date.now() + ms.globalSettings.get('Redraw Time') * 1000);
	ms.timerG.stop('Redraw');
};

ms.synthesizer.prototype.setExample = function(example) {	
	if (ms.globalSettings.get('Use Boundary Cells')) {
		alert('Guidelines does not use boundary cells.');
	}
	ms.highlightedElement = null;

	var shapes = example.getShapes();
	this.is3D = shapes[0].is3D;
	var expectedMode = this.is3D ? ms.view.modes.VIEW3D : ms.view.modes.GRID;
	if (this.observer.mode != expectedMode) {
		this.observer.setMode(expectedMode);
		this.viewport = this.observer.getViewport();
		this.cameraController.activate();
	}

	var guideClassifier = new ms.classifier();
	example.updateFaces();
	if (example.solution) {
		guideClassifier.importSolution(example.solution);
		guideClassifier.is3D = example.shapes[0].is3D;
	} else {
		guideClassifier.setShapes(shapes);

		/* var exported = guideClassifier.hierarchy.export();
		var imported = ms.networkHierarchy.import(exported);
		guideClassifier.hierarchy = imported; */
	}
	guideClassifier.boundaryGroups = guideClassifier.hierarchy.boundaryGroups;
	this.is3D = guideClassifier.is3D;
	this.mutator = new ms.guideMutator(guideClassifier, this.timer, this.notify.bind(this));
};

ms.synthesizer.prototype.initializeMutationArea = function() {
	if (ms.globalSettings.get('Incremental Mutation')) {
		this.mutationOrigin = ms.globalSettings.get('Empty Border') ? [1, 1, 0] : [0, 0, 0];
		var mutationSize = ms.globalSettings.get('Extents').slice();
		mutationSize[0] -= 2;
		mutationSize[1] -= 2;		
		if (!this.is3D) {
			mutationSize[2] = 1;
		}
		this.mutationArea.setExtents(this.mutationOrigin, mutationSize);
	} else {
		this.mutationArea = new ms.mutationArea(this.model, this.mutator.nodeStats);
		this.startMutation();
		this.mutator.unresolve();
	}
};

ms.synthesizer.prototype.synthesize = function(extents, keepPaused, callback) {
	if (!this.is3D) {
		extents = extents.slice();
		extents[2] = 1;
	}
	this.timer.start('everything');
	ms.util.updateRandomMode();

	this.model = new ms.model(extents);
	this.notify(true);
	this.callback = callback;
	
	ms.node.count = 0;
	ms.guideMutator.taskCount = 0;
	ms.edgeType.count = 0;
	ms.guideMutator.mutationCount = 0;

	this.mutationArea = new ms.mutationArea(this.model, this.mutator.nodeStats);
	this.numAttempts = 0;
	var redrawTime = ms.globalSettings.get('Redraw Time') * 1000;
	var maxTime = ms.globalSettings.get('Max Time') * 1000;
	this.mutator.setBreakTime(Date.now() + redrawTime);
	this.mutator.setEndTime(Date.now() + 0.9 * maxTime, Date.now() + maxTime);
	this.initializeMutationArea();

	this.timedOut = false;
	if (keepPaused) {
		this.startMutation();
		this.pause();
	} else {
		this.mutateUntilDone();
	}
};

ms.synthesizer.prototype.finish = function() {
	this.timer.stop('everything');
	this.timer.save();
	window.console.log(this.timer.reportMean());
	this.model.setFinished(true);
	this.notify(true);
	this.callback(this.mutator.report());
};

ms.synthesizer.prototype.mutateUntilDone = function() {
	if (this.timedOut && this.timer.has('Timed out')) {
		// We check that it has timed out as it may have been asynchonrously cleared.
		this.timer.stop('Timed out');
	}
	if (ms.globalSettings.get('Incremental Mutation')) {
		var extents = this.model.getExtents();
		var SIZE = 1; // ms.globalSettings.get('Mutation Size');
		if (this.mutationOrigin[2] + SIZE[2] > extents[2]) {
			if (ms.globalSettings.get('Max Time Enabled')) {
				this.initializeMutationArea()
			} else {
				this.finish();
				return;
			}
		}
	} else {
		if (this.mutator.getStatus() != ms.guideMutator.status.UNRESOLVED) {
			this.notify();
			this.timer.save();
			window.console.log(this.timer.reportMean());
			return;
		}
	}
	this.mutate();
	if (this.mutator.getStatus() != ms.guideMutator.status.PAUSED && 
	    this.mutator.getStatus() != ms.guideMutator.status.FINISHED) {
		this.timer.start('Timed out');
		this.timedOut = true;
		window.setTimeout(this.mutateUntilDone.bind(this), 0);
	}
};

ms.synthesizer.prototype.pause = function() {
	this.mutator.pause();
};

ms.synthesizer.prototype.resume = function() {
	this.mutator.unresolve();
	window.setTimeout(this.mutateUntilDone.bind(this), 0);
};

// Step through a certain number of tasks.
ms.synthesizer.prototype.step = function(taskNum) {
	this.mutator.step(taskNum);
	this.resume();
};

ms.synthesizer.prototype.startMutation = function() {
	this.timer.start('Start Mutation');
	this.mutationArea.free();
	this.mutator.save();
	var initialized = false;
	while (!initialized) {
		this.initializeMutationArea();
		this.mutator.reset();
		this.mutator.seed(this.mutationArea);
		this.mutationArea.free();
		initialized = this.mutator.initializeModel(this.model);
	}
	this.mutator.save();
	this.timer.stop('Start Mutation');
};


ms.synthesizer.prototype.mutate = function() {
	var statusType = ms.guideMutator.status;
	var status = this.mutator.getStatus();
	if (status != statusType.UNRESOLVED && status != statusType.PAUSED) {
		this.startMutation();
	}
	var status = this.mutator.resolve(this.model);
	switch (status) {
		case statusType.UNRESOLVED:
			this.notify(ms.globalSettings.get('Show Faces'));
			break;
		case statusType.FINISHED:
			this.finish();
			break;
	}
};

ms.synthesizer.prototype.onmousedown = function(x, y) {
	var viewport = this.observer.getViewport();
	var modelPosition = viewport.inverseTransform(new ms.vec2(x, y));
	modelPosition.y = this.model.extents[1] - modelPosition.y;
	var rayCast = this.model.rayCastLeft(modelPosition);
	if (rayCast) {
		var face = rayCast.endpoint.getFace();
		face.print();
		var enclosed = face.enclosedFace();
		if (enclosed) {
			// enclosed.print();
		}
	}
	/* this.model.getVertices().forEach(function(vertex) {
		var circle = vertex.getEndpoints()[0].getCircle();
		var r = Math.max(circle ? circle.getRadius() : 0, 0.1);
		if (vertex.getPosition().distance2(modelPosition) < r * r) {
			vertex.print();
			debugger;
		}
	}); */
};

ms.synthesizer.prototype.exportOutput = function() {
	var xml = this.mutator.classifier.xml;
	return ms.exporter.export(this.model.nodeStats, xml, this.viewport);
};
