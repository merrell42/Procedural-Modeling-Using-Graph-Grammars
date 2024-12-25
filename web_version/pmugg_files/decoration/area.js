ms.area = function(color, borderColor, onChange) {
	this.borderColor = borderColor;
	this.element = null;
	
	this.properties = {};
	this.onChange = onChange;
	// The color of the area.
	this.addProperty(new ms.settingProperty('color', color, new ms.settingNumber(true), onChange));

	var options = ms.brush.getImageGroups();
	this.addProperty(new ms.settingProperty('Image', 'None', new ms.settingNumber(true), onChange));
	
	// The width of the tiles in grid length.
	this.addProperty(new ms.settingProperty('Width', 5, new ms.settingNumber(), onChange));
	
	// The height of the tiles in grid length.
	this.addProperty(new ms.settingProperty('Height', 5, new ms.settingNumber(), onChange));
	
	// If the origin is fixed or at the left most vertex.
	this.addProperty(new ms.settingProperty('Fixed Origin', false, new ms.settingBoolean(), onChange));
	
	// If this is a special area on the boundary.
	this.addProperty(new ms.settingProperty('Boundary', false, new ms.settingBoolean(), onChange));
	
	this.faceType = new ms.faceType3D(this, ms.vec3.Z_HAT);
	
	this.id = ms.area.counter++;
};

ms.area.counter = 0;

ms.area.prototype.addProperty = function(property) {
	this.properties[property.getLabel()] = property;
};

ms.area.prototype.copyProperties = function() {
	var result = {};
	for (var key in this.properties) {
		result[key] = this.properties[key].copy();
	}
	return result;
};

// This gets the whole property. If you just want the value use get.
ms.area.prototype.getWholeProperty = function(key) {
	return this.properties[key];
};

ms.area.prototype.get = function(key) {
	if (!this.properties[key]) {
		return null;
	}
	return this.properties[key].getValue();
};

ms.area.prototype.set = function(key, value) {
	if (!this.properties[key]) {
		return;
	}
	this.properties[key].setValue(value);
};

ms.area.prototype.getId = function() {
	return this.id;
};

ms.area.prototype.getColor = function() {	
	return this.properties['color'].getValue();
};

ms.area.prototype.getBorderColor = function() {
	return this.borderColor;
};

ms.area.prototype.getFaceType = function() {
	return this.faceType;
};

ms.area.prototype.export = function() {
	var result = {};
	Object.entries(this.properties).forEach(function(property) { result[property[0]] = property[1].getValue(); });
	return JSON.stringify(result);
};

ms.area.import = function(version, values, onChange) {
	var area = new ms.area('', '', onChange);
	if (version <= 4) {
		area.import(values);
	} else {
		var value = JSON.parse(values.shift());
		Object.entries(value).forEach(function(entry) {
			area.set(entry[0], entry[1]);
		});
	}
	return area;
};

ms.area.prototype.import = function(data) {
	this.set('color', data.shift());
	this.borderColor = data.shift();
};
