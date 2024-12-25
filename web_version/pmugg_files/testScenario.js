ms.testScenario = function(name) {
	this.name = name;
	this.shapes = [];
	this.screenSaver = null;
	this.rows = 0;
	this.columns = 0;
	this.variables = [];
	this.runCount = 0;
	this.variableCombinations = 1;
};

ms.testScenario.prototype.getName = function() {
	return this.name;
};

ms.testScenario.prototype.addShape = function(shape) {
	this.shapes.push(shape);
};

ms.testScenario.prototype.setScreenSaver = function(screenSaver, rows, columns) {
	this.screenSaver = screenSaver;
	this.rows = rows;
	this.columns = columns;
};

ms.testScenario.prototype.setVariable = function(name, values) {
	if (!ms.globalSettings.hasProperty(name)) {
		alert('Settings does not have the property: ' + name);
	}
	this.variables.push({name: name, values: values});
	this.variableCombinations *= values.length;
};

ms.testScenario.prototype.reset = function() {
	this.runCount = 0;
	this.screenSaver && this.screenSaver.reset(this.rows, this.columns);
};

ms.testScenario.prototype.logSolution = function(solution) {
	this.shapes[this.runCount - 1].solution = solution;
};

ms.testScenario.prototype.logReport = function(report) {
	var shapeCount = this.shapes.length;
	report.name = this.shapes[(this.runCount - 1) % shapeCount].name;
	this.screenSaver && this.screenSaver.logReport(report);
};

ms.testScenario.prototype.run = function(runShapeFunc, solveOnly) {
	var shapeCount = this.shapes.length;
	if (this.runCount > 0) {
		if (this.screenSaver) {
			this.screenSaver.save();
		} else {
			var name = this.shapes[(this.runCount - 1) % shapeCount].name;
			if (solveOnly || !ms.globalSettings.get('Pause when finished')) {
				console.log(name);
			} else {
				alert(name);
			}
		}
	}
	var finished = (this.runCount >= this.variableCombinations * shapeCount);
	if (!finished) {
		if (this.runCount % shapeCount == 0) {
			if (this.runCount > 0) {
				this.screenSaver && this.screenSaver.flush();
			}
			var combination = Math.floor(this.runCount / shapeCount);
			var imageNameEnd = '';
			for (var i = 0; i < this.variables.length; i++) {
				var variable = this.variables[i];
				var value = variable.values[combination % variable.values.length];
				ms.globalSettings.set(variable.name, value);
				if (variable.values.length > 1) {
					imageNameEnd += ' ' + variable.name + '_' + value;
				}
				combination = Math.floor(combination / variable.values.length);
			}
			this.screenSaver && this.screenSaver.setImageName(this.name, imageNameEnd);
		}
		var shape = this.shapes[this.runCount % shapeCount];
		this.runCount++;
		ms.timerG.clear();
		runShapeFunc(shape);	
	} else {
		this.screenSaver && this.screenSaver.flush();
		if (solveOnly) {
			// var compressed = LZString.compress(JSON.stringify(this.shapes));
			var compressed = LZString.compressToBase64(JSON.stringify(this.shapes));
			navigator.clipboard.writeText('ms.mvpSolutions = \'' + compressed + '\';');
			console.log('ms.mvpSolutions = \'' + compressed + '\';');
		}
	}
};
