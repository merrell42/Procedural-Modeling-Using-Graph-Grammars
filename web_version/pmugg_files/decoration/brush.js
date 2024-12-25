ms.brush = function(color, borderColor, onChange) {
	this.properties = {};
	this.onChange = onChange;

	var options = ms.brush.getImageGroups();
	this.addProperty(new ms.settingProperty('Image', '', new ms.settingNumber(true), onChange));
	
	// The color of the brush.
	this.addProperty(new ms.settingProperty('color', color, new ms.settingNumber(true), onChange));

	// Whether the line is tiled. Otherwise it will extend the full length of the line.
	this.addProperty(new ms.settingProperty('Tiled', true, new ms.settingHidden(), onChange));

	// The length of the tiles.
	this.addProperty(new ms.settingProperty('Tile Length', 1, new ms.settingHidden(), onChange));

	// The width of the line as it is displayed.
	this.addProperty(new ms.settingProperty('Width', 0.05, new ms.settingHidden(), onChange));

	// The thickness of the line for the purpose of intersecting it with other shapes.
	this.addProperty(new ms.settingProperty('Thickness', 0.05, new ms.settingHidden(), onChange));
	
	// The minimum length of the line.
	this.addProperty(new ms.settingProperty('Min Length', ms.transistor.defaultLengthMin, new ms.settingNumber(), onChange));

	// The maximum length of the line.
	this.addProperty(new ms.settingProperty('Max Length', ms.transistor.defaultLengthMax, new ms.settingNumber(), onChange));
	
	// The amount that the edge can be bent at the cell boundaries in degrees.
	this.addProperty(new ms.settingProperty('Bend', 10, new ms.settingNumber()));

	// If the min and max length are strictly set.
	this.addProperty(new ms.settingProperty('Strict Length', false, new ms.settingHidden(), onChange));

	// The power in the power law distribution.
	this.addProperty(new ms.settingProperty('Power', -1, new ms.settingHidden(), onChange));

	// Whether the length are computed from an exponential distribution or a uniform distribution.
	this.addProperty(new ms.settingProperty('Power Law', false, new ms.settingHidden()));


	// The amount the tiled shape is offset orthogonal to the edge.
	this.addProperty(new ms.settingProperty('Offset', 0, new ms.settingHidden(), onChange));

	// Whether the edges are rigid and fixed. Meaning it cannot stretch or tile.
	this.addProperty(new ms.settingProperty('Rigid', false, new ms.settingBoolean()));

	// Whether the edges are rigid. and tiles. Meaning it can be tiled in integer increments.
	this.addProperty(new ms.settingProperty('Rigid Tiled', false, new ms.settingHidden()));
	
	// Rotate the tiled image 180 degrees.
	this.addProperty(new ms.settingProperty('Flip', false, new ms.settingHidden()));
	
	// Whether the edges are separated into different types based on their angle.
	this.addProperty(new ms.settingProperty('Classified by angle', true, new ms.settingHidden()));
	
	// Whether the edges are bidirectional meaning that edges 180 degrees apart are identical.
	this.addProperty(new ms.settingProperty('Bidirectional Edges', true, new ms.settingHidden()));
	
	// Only keep for legacy reasons.
	this.addProperty(new ms.settingProperty('Synthetic', true, new ms.settingHidden()));
	
	// Deprecated: Whether this edge is on a ground line.
	this.addProperty(new ms.settingProperty('Grounded', false, new ms.settingHidden()));
	
	// Whether this represents the boundary of the screen.
	this.addProperty(new ms.settingProperty('Boundary', false, new ms.settingBoolean()));

	// Whether this is always fully connected.
	this.addProperty(new ms.settingProperty('Fully Connected', false, new ms.settingBoolean()));

	// Whether this brush can form loops.
	this.addProperty(new ms.settingProperty('Loopy', true, new ms.settingBoolean()));
	
	// Sinusoid or Low-pass.
	this.addProperty(new ms.settingProperty('Sinusoid', false, new ms.settingHidden()));
	
	// The width of the low-pass filter in the bending.
	this.addProperty(new ms.settingProperty('Bend Low-Pass', 5, new ms.settingHidden()));
	
	// The lines are divided into a set of straight components that are bent. This is the length of each component. Shorter length means more bending.
	this.addProperty(new ms.settingProperty('Bend Length', 0.1, new ms.settingHidden()));
	
	// The period of a sinusoid curve.
	this.addProperty(new ms.settingProperty('Bend Period', 1, new ms.settingHidden()));
	
	// How many smaller levels of noise to add.
	this.addProperty(new ms.settingProperty('Bend Harmonics Levels', 1, new ms.settingHidden()));
	
	// The color of the border of the brush in the brush selector.
	this.addProperty(new ms.settingProperty('borderColor', borderColor, new ms.settingHidden(), onChange));	
	
	// Whether or not to display the brush.
	this.addProperty(new ms.settingProperty('Visible', true, new ms.settingBoolean()));	
};

ms.brush.getImageGroups = function() {
	var imageHeader = document.getElementById('images');
	var groups = imageHeader.children;
	var options = ['None'];
	for (var i = 0; i < groups.length; i++) {
		options.push(groups[i].id);
	}
	return options;
};

ms.brush.properties = [];
ms.brush.properties[2] = ["Rigid", "Classified by angle", "Bidirectional Edges", "Synthetic", "Bend", "color", "borderColor"];
ms.brush.properties[3] = ["Rigid", "Classified by angle", "Bidirectional Edges", "Synthetic", "Bend", "color", "borderColor"];
ms.brush.properties[4] = ["Rigid", "Classified by angle", "Bidirectional Edges", "Synthetic", "Grounded", "Bend", "color", "borderColor"];

ms.brush.prototype.get = function(key) {
	if (!this.properties[key]) {
		return null;
	}
	return this.properties[key].getValue();
};

ms.brush.prototype.set = function(key, value) {
	if (!this.properties[key]) {
		return;
	}
	this.properties[key].setValue(value);
};

ms.brush.prototype.addProperty = function(property) {
	this.properties[property.getLabel()] = property;
};

// This gets the whole property. If you just want the value use get.
ms.brush.prototype.getWholeProperty = function(key) {
	return this.properties[key];
};

ms.brush.prototype.getColor = function() {
	return this.properties.color.getValue();
};

ms.brush.prototype.getBorderColor = function() {
	return this.properties.borderColor.getValue();
};

ms.brush.prototype.hasTile = function() {
	var tile = this.get('Image');
	return tile && tile != 'None';
};

ms.brush.prototype.copyProperties = function() {
	var result = {};
	for (var key in this.properties) {
		result[key] = this.properties[key].copy();
	}
	return result;
};

ms.brush.prototype.importXml = function(brush) {
	var self = this;
	var attrs = brush['@attributes'];
	Object.keys(attrs).forEach((key0) => {
		if (key0 == 'types' || key0 == 'name') {
			return;
		}
		var key = key0.replace('_', ' ');
		self.set(key, JSON.parse(attrs[key0]));
	});
};

ms.brush.prototype.export = function() {
	var result = {};
	Object.entries(this.properties).forEach(function(property) { result[property[0]] = property[1].getValue(); });
	return JSON.stringify(result);
};

ms.brush.import = function(version, values, onChange) {
	var brush = new ms.brush('', '', onChange);
	if (version <= 4) {
		brush.import(version, values);
	} else {
		var value = JSON.parse(values.shift());
		Object.entries(value).forEach(function(entry) {
			brush.set(entry[0], entry[1]);
		});
	}
	return brush;
};

ms.brush.prototype.import = function(version, values) {
	var self = this;
	ms.brush.properties[version].forEach(function(key) {
		self.properties[key].setValue(values.shift());
	});
};

ms.brush.bendSequence = function(brush, n) {
	var isSinusoid = brush && brush.get('Sinusoid');	
	if (isSinusoid) {
		var bendLength = brush ? brush.get('Bend Length') : 0.1;
		var frequency = brush ? (1 / brush.get('Bend Period')) : 1;
		var levels = brush ? brush.get('Bend Harmonics Levels') : 1;
		var maxBend = (brush ? brush.get('Bend') : 10) * ms.globalSettings.get('Bend Multiplier') * Math.PI / 180 * bendLength;
		var angularFreq = 2 * Math.PI * bendLength * frequency;

		var sinusoid = [];
		for (var i = 0; i < n; i++) {
			sinusoid.push(0);
		}
		for (var level = 0; level < levels; level++) {
			var phase = Math.PI * Math.random();
			for (var i = 0; i < n; i++) {
				sinusoid[i] = sinusoid[i] + maxBend * Math.sin(angularFreq * i + phase);
			}
			maxBend /= 2;
			angularFreq *= 2;
		}
		return sinusoid;
	} else {	
		var w = brush ? brush.get('Bend Low-Pass') : 1;
		var intensity = (brush ? brush.get('Bend') : 10) * ms.globalSettings.get('Bend Multiplier') * Math.PI / 180; // * bendLength; // Not sure if the bendLength belongs or not.
		
		// Returns random noise passed through a low pass filter.
		// n is the length of the returned sequence.
		// w is the length of the low pass filter.	
		var noise = [];
		var filtered = [];
		for (var i = 0; i < n + w - 1; i++) {
			noise.push(2 * intensity * (Math.random() - 0.5));
		}
		var sum = 0;
		for (var i = 0; i < w; i++) {
			sum += noise[i];
		}
		filtered[0] = sum / w;
		for (var i = 1; i < n; i++) {
			sum += noise[i + w - 1];
			sum -= noise[i - 1];
			filtered[i] = sum / w;
		}
		return filtered;
	}
};
