ms.timer = function() {
	this.accumulator = {};
	this.clock = Date.now();
	this.savedTimings = [];
};

// This prints a message saying how long it has been seen this function was
// last called in milliseconds. Convenient for timing things.
ms.timer.prototype.tick = function(message) {
	var time = Date.now();
	if (message) {
		window.console.log(message + ': ' + (time - this.clock));
	}
	this.clock = time;
};

ms.timer.prototype.clear = function() {
	this.accumulator = {};
	this.savedTimings = [];
};


ms.timer.prototype.has = function(type) {
	return this.accumulator.hasOwnProperty(type);
};

ms.timer.prototype.start = function(type) {
	if (!this.accumulator.hasOwnProperty(type)) {
		this.accumulator[type] = {clock: 0, sum: 0};
	}
	this.accumulator[type].clock = Date.now();
};

ms.timer.prototype.stop = function(type) {
	var time = Date.now();
	this.accumulator[type].sum += time - this.accumulator[type].clock;
};

ms.timer.prototype.getTiming = function(type) {
	var sum = 0;
	this.savedTimings.forEach(function(timing) {
		sum += timing.hasOwnProperty(type) ? timing[type].sum : 0;
	});
	return sum;
};

ms.timer.prototype.reportCurrent = function() {
	var summary = '';
	Object.keys(this.accumulator).forEach((key) => {
		summary += key + ': ' + this.accumulator[key].sum + '\n';
	});
	return summary;
};

ms.timer.prototype.reportMean = function() {
	var types = [];
	for (var i = 0; i < this.savedTimings.length; i++) {
		var keys = Object.keys(this.savedTimings[i]);
		keys.forEach(function(key) {
			types.push(key);
		});
	}
	var summary = '';
	var self = this;
	var reports = types.map(function(type) {
		return {type: type, timing: self.getTiming(type)};
	});
	reports.sort(function(a, b) {
		return b.timing - a.timing;
	});
	if (ms.globalSettings.get('Report Timings')) {
		reports.forEach(function(report) {
			summary += report.type + ': ' + (report.timing / self.savedTimings.length).toFixed(1) + '\n';
		});
	}
	summary += 'Task Count: ' + ms.guideMutator.taskCount + '\n';
	summary += 'Task frequency: ' + (ms.guideMutator.taskCount / this.getTiming('everything')).toFixed(5) + '\n';
	summary += 'Number of timings: ' + this.savedTimings.length;
	return summary;
};

ms.timer.prototype.save = function() {
	this.savedTimings.push(this.accumulator);
	this.accumulator = {};
};

// Global Variable
ms.timerG = new ms.timer();
