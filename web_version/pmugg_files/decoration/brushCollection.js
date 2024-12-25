ms.brushCollection = function() {
	this.brushes = [];
	this.isArea = false;
};

ms.brushCollection.prototype.createDefault = function(onChange) {
	this.brushes.push(new ms.brush('#2896ff', '#055', onChange));
	this.brushes.push(new ms.brush('#ff3232', '#500', onChange));
	this.brushes[1].set('Rigid', true);
	this.isArea = false;
};

ms.brushCollection.prototype.createArea = function(onChange) {
	this.brushes.push(new ms.area('rgb(192, 240, 255)', '#055', onChange));
	this.brushes.push(new ms.area('rgb(255, 192, 192)', '#500', onChange));
	this.brushes.push(new ms.area('rgb(192, 255, 192)', '#050', onChange));
	this.isArea = true;
};

ms.brushCollection.prototype.export = function() {
	var result = [this.brushes.length];
	for (var i = 0; i < this.brushes.length; i++) {
		result.push(this.brushes[i].export());
	}
	return result;
};

ms.brushCollection.prototype.import = function(version, values, isArea, onChange) {	
	this.brushes = [];
	var numBrushes = values.shift();
	for (var i = 0; i < numBrushes; i++) {
		if (isArea) {
			this.brushes.push(ms.area.import(version, values, onChange));
		} else {
			this.brushes.push(ms.brush.import(version, values, onChange));
		}
	}
};

ms.brushCollection.prototype.addBrush = function(onChange) {
	var color = ms.brushCollection.getColor(this.brushes.length + 1, this.isArea);
	if (this.isArea) {
		this.brushes.push(new ms.area(color, '#222', onChange));
	} else {
		this.brushes.push(new ms.brush(color, '#222', onChange));
	}
};

ms.brushCollection.prototype.addThisBrush = function(brush) {
	this.brushes.push(brush);
};

ms.brushCollection.prototype.removeBrush = function() {
	this.brushes.pop();
};

ms.brushCollection.prototype.getBrushes = function() {
	return this.brushes;
};

ms.brushCollection.prototype.getIndex = function(brush) {
	return this.brushes.indexOf(brush);
};

ms.brushCollection.prototype.getFromIndex = function(index) {
	return this.brushes[index];
};

ms.brushCollection.getColor = function(type, isArea) {
	var color;
	// switch ((type + 8) % 23) {
  if (!isArea) {
		switch (type % 23) {
			case 0: color = '#444'; break;
			case 1: color = '#04f'; break;
			case 2: color = '#0ff'; break;
			case 3: color = '#0f0'; break;
			case 4: color = '#8f0'; break;
			case 5: color = '#ff0'; break;
			case 6: color = '#f50'; break;
			case 7: color = '#f00'; break;
			case 8: color = '#f04'; break;
			case 9: color = '#f0f'; break;
			case 10: color = '#555'; break;
			case 11: color = '#00f'; break;
			case 12: color = '#03f'; break;
			case 13: color = '#055'; break;
			case 14: color = '#052'; break;
			case 15: color = '#060'; break;
			case 16: color = '#0f8'; break;
			case 17: color = '#350'; break;
			case 18: color = '#550'; break;
			case 19: color = '#620'; break;
			case 20: color = '#700'; break;
			case 21: color = '#602'; break;
			case 22: color = '#505'; break;
		}
	} else {
		switch (type % 23) {
			case 0: color = '#999'; break;
			case 1: color = '#79f'; break;
			case 2: color = '#7dd'; break;
			case 3: color = '#7f9'; break;
			case 4: color = '#7f7'; break;
			case 5: color = '#ba7'; break;
			case 6: color = '#ff7'; break;
			case 7: color = '#fb7'; break;
			case 8: color = '#f77'; break;
			case 9: color = '#f79'; break;
			case 10: color = '#d7d'; break;
			case 11: color = '#bbb'; break;
			case 12: color = '#77d'; break;
			case 13: color = '#78c'; break;
			case 14: color = '#7bb'; break;
			case 15: color = '#7b7'; break;
			case 16: color = '#7c7'; break;
			case 17: color = '#8b7'; break;
			case 18: color = '#bb7'; break;
			case 19: color = '#c27'; break;
			case 20: color = '#d77'; break;
			case 21: color = '#c77'; break;
			case 22: color = '#b7b'; break;
		}
	}
	return color;
};