ms.vertexDecoration = function(onChange) {
	this.properties = {};
	this.onChange = onChange;

	var options = ms.brush.getImageGroups();
	this.addProperty(new ms.settingProperty('Image', '', new ms.settingNumber(true), onChange));

	// The width of the image in pixels.
	this.addProperty(new ms.settingProperty('Width', 0.5, new ms.settingNumber(), onChange));

	// The height of the image in pixels.
	this.addProperty(new ms.settingProperty('Height', 0.5, new ms.settingNumber(), onChange));

	// Whether to fix the aspect ratio as the width and height are modified.
	this.addProperty(new ms.settingProperty('Fix Aspect Ratio', true, new ms.settingBoolean(), onChange));

	// How desirable or undesirable the vertex is in the cost function.
	this.addProperty(new ms.settingProperty('Desirability', 0, new ms.settingNumber()));

	// How much to offset the center of the image in the x and y directions.
	this.addProperty(new ms.settingProperty('Center X', 0, new ms.settingNumber(), onChange));
	this.addProperty(new ms.settingProperty('Center Y', 0, new ms.settingNumber(), onChange));

	// Where to put the anchor point. This the user interaction with resizing, not the actual display.
	this.addProperty(new ms.settingProperty('Anchor X', 0, new ms.settingNumber(), onChange));
	this.addProperty(new ms.settingProperty('Anchor Y', 0, new ms.settingNumber(), onChange));
};

ms.vertexDecoration.prototype.get = function(key) {
	return this.properties[key].getValue();
};

ms.vertexDecoration.prototype.set = function(key, value) {
	if (!this.properties[key]) {
		return;
	}
	this.properties[key].setValue(value);
};

// This gets the whole property. If you just want the value use get.
ms.vertexDecoration.prototype.getWholeProperty = function(key) {
	return this.properties[key];
};

ms.vertexDecoration.prototype.addProperty = function(property) {
	this.properties[property.getLabel()] = property;
};

ms.vertexDecoration.prototype.copyProperties = function() {
	var result = {};
	for (var key in this.properties) {
		result[key] = this.properties[key].copy();
	}
	return result;
};

ms.vertexDecoration.prototype.hasImage = function() {
	var image = this.get('Image');
	return image && image != 'None';
};

ms.vertexDecoration.prototype.isEmpty = function() {
	return !this.hasImage() && this.get('Desirability') == 0;
};

ms.vertexDecoration.prototype.importXml = function(vDecoration) {
	var self = this;
	var attrs = vDecoration['@attributes'];
	Object.keys(attrs).forEach((key0) => {
		if (key0 == 'types' || key0 == 'name') {
			return;
		}
		var key = key0.replace('_', ' ');
		self.set(key, JSON.parse(attrs[key0]));
	});
};

ms.vertexDecoration.prototype.export = function() {
	var result = {};
	Object.entries(this.properties).forEach(function(property) { result[property[0]] = property[1].getValue(); });
	return result;
};

ms.vertexDecoration.import = function(value, onChange) {	
	var decoration = new ms.vertexDecoration(onChange);
	Object.entries(value).forEach(function(entry) {
		if (entry[0] != 'x' && entry[0] != 'y') {
			decoration.set(entry[0], entry[1]);
		}
	});
	return decoration;
};
