ms.settings = function() {
	this.properties = {};
	
	// The size of the output model.
	this.addProperty(new ms.settingProperty('Extents', [30, 20, 10], new ms.settingExtent()));

	// Whether to mutate the model in incremental parts instead of all at once.
	// TODO: Maybe delete.
	this.addProperty(new ms.settingProperty('Incremental Mutation', true, new ms.settingHidden()));
	
	// Whether to add boundary cells in between the main columns. 
	this.addProperty(new ms.settingProperty('Use Boundary Cells', false, new ms.settingHidden()));
	
	// The name of the example model. Only used in model-piece mode.
	// this.addProperty(new ms.settingProperty('Model Piece Example', 'ms.L_SHAPE_EXAMPLE2', new ms.settingHidden())); // ms.ARCH_EXAMPLE
	this.addProperty(new ms.settingProperty('Model Piece Example', 'ms.ESCHER_EXAMPLE', new ms.settingHidden())); // ms.ARCH_EXAMPLE
	
	// The amount of time in seconds to wait before redrawing the model.
	this.addProperty(new ms.settingProperty('Redraw Time', 0.1, new ms.settingNumber(), null, true));
	
	// Whether to allow the generated models to be rotated copies of the example.
	this.addProperty(new ms.settingProperty('Allow Rotations', false, new ms.settingBoolean()));
	
	// Whether to bend the lines.
	this.addProperty(new ms.settingProperty('Bend Lines', false, new ms.settingBoolean()));

	// Whether to check for intersections when bending the lines.
	this.addProperty(new ms.settingProperty('Careful Bending', true, new ms.settingHidden()));
	
	// When to start bending the lines.
	this.addProperty(new ms.settingProperty('Bend Time Start', 0, new ms.settingNumber(), null, true));

	// Whether to allow the generated models to be scaled copies of the example. Only applies to rigid lines.
	this.addProperty(new ms.settingProperty('Allow Scaling', false, new ms.settingHidden()));
	
	// The amount to scale the imported shapes.
	this.addProperty(new ms.settingProperty('Import Scale', 1.0, new ms.settingHidden()));
	
	// When adding a random vertex to an edge, do we consider options that include a straight line somewhere.
	this.addProperty(new ms.settingProperty('Many Rigid Bent Options', true, new ms.settingHidden()));
	
	// When adding a random vertex to an edge, do we consider options that include a straight line somewhere.
	this.addProperty(new ms.settingProperty('Many Flexible Bent Options', false, new ms.settingHidden()));
	
	// Whether or not to randomly remove good vertices.
	this.addProperty(new ms.settingProperty('Remove Good Vertices', false, new ms.settingHidden()));
	
	// The preferred range that the edge can be bent at the cell boundaries in degrees. The edge can be
	// bent further if there is a good reason.
	this.addProperty(new ms.settingProperty('Bend Multiplier', 1, new ms.settingNumber()));
	
	// If we should snap the vertices to a grid when making shapes.
	this.addProperty(new ms.settingProperty('Snap Grid', true, new ms.settingHidden()));
	
	// If we should snap the vertices to a grid when making shapes.
	this.addProperty(new ms.settingProperty('Snap Grid Spacing', 20, new ms.settingHidden()));
	
	// The size of the major grid lines when making spaces. This is the size of the grid used in grid view.
	this.addProperty(new ms.settingProperty('Major Grid Spacing', 40, new ms.settingHidden()));

	// How close the angle need to be before they can be snapped together
	this.addProperty(new ms.settingProperty('Snap Angle', 20, new ms.settingHidden()));

	// The number of times to free vertices.
	this.addProperty(new ms.settingProperty('Mutator Effort Limit', 20, new ms.settingHidden()));

	// The number of vertices to free at once.
	this.addProperty(new ms.settingProperty('Vertices to Free', 1, new ms.settingHidden()));

	// Prefer picking the ground over other faces.
	this.addProperty(new ms.settingProperty('Prefer Ground', 0.9, new ms.settingNumber(), null, true));

	// If the border should be kept empty.
	this.addProperty(new ms.settingProperty('Empty Border', true, new ms.settingHidden()));

	// The beta value in the optimizations. Higher value means we accept fewer low cost models.
	this.addProperty(new ms.settingProperty('Beta', 1, new ms.settingHidden()));

	// The amount of effort to expend in snapping before giving up.
	this.addProperty(new ms.settingProperty('Snapping Effort Limit', 100, new ms.settingHidden()));

	// Weights of different cost terms in the optimizer.
	this.addProperty(new ms.settingProperty('Bad Vertex', 10, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Endpoint Conflict', 1, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Endpoint Conflict Base', 1.1, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Flexible Line Length', 50, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Rigid Line Length', 500, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Ideal Length', 0.5, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Face', 5, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Completion', 4, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Valence', 0, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Component', 20, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('State Density', 2, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Component Scale', 10, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Component Base', 1.5, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Face Base', 1.1, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Visibility', 0, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Closible Endpoint Index', 1, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Closible Angle', 200, new ms.settingHidden()));

	// The desired number of lines. Above this it makes no differences
	this.addProperty(new ms.settingProperty('Desired Lines', 200, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Desired Lines Cost', 10000, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Desired Vertex Weight', 100, new ms.settingHidden()));

	this.addProperty(new ms.settingProperty('Free Vertices', true, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Kill Junctions', true, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Full Move', true, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Modify center', false, new ms.settingHidden()));
	this.addProperty(new ms.settingProperty('Immutable Ground', true, new ms.settingHidden()));

	this.addProperty(new ms.settingProperty('Line Weld Probability', 0.7, new ms.settingHidden()));

	// Whether to trigger the debugger on an alert.
	this.addProperty(new ms.settingProperty('Node Debug', -1, new ms.settingNumber(), null, true));;

	// Whether to trigger the debugger on an alert.
	this.addProperty(new ms.settingProperty('Debug Alerts', false, new ms.settingBoolean(), null, true));

	// If true, stop synthesizing based on the maximum time.
	this.addProperty(new ms.settingProperty('Max Time Enabled', false, new ms.settingHidden()));
	
	// The maximum time for synthesizing.
	this.addProperty(new ms.settingProperty('Max Time', 10, new ms.settingHidden()));
	
	// The maximum number of iterations.
	this.addProperty(new ms.settingProperty('Max Iterations', 500, new ms.settingNumber()));
	
	// Maximum number of generations.
	this.addProperty(new ms.settingProperty('Generations', 5, new ms.settingNumber(), null, true));
	
	// Maximum number of connectors to allow.
	this.addProperty(new ms.settingProperty('Max Connectors', 100, new ms.settingNumber(), null, true));
	
	// Whether or not winding past 360 degrees is allowed.
	this.addProperty(new ms.settingProperty('Winding Enabled', true, new ms.settingBoolean(), null, true));
	
	// Whether or not to allowing simplification using branching.
	this.addProperty(new ms.settingProperty('Stubs Enabled', false, new ms.settingBoolean(), null, true));
	
	// Whether or not to allow new rings to divide and match existing rings.
	this.addProperty(new ms.settingProperty('Match Backwards', true, new ms.settingBoolean(), null, true));
	
	// If the chain map should be used to remove nodes early.
	this.addProperty(new ms.settingProperty('Use Chain Map', false, new ms.settingBoolean(), null, true));
	
	// Whether or not to allow new rings to divide and match existing rings.
	this.addProperty(new ms.settingProperty('Monotonic', true, new ms.settingBoolean(), null, true));
	
	// Inactivate hierarchy nodes with a monotonic loopable.
	// this.addProperty(new ms.settingProperty('Monotonic Inactive', false, new ms.settingHidden()));
	
	// Group the connectors into super edges.
	this.addProperty(new ms.settingProperty('Super Edges', true, new ms.settingBoolean(), null, true));

	// Show or hide inactive hierarchy nodes.
	this.addProperty(new ms.settingProperty('Show Inactives', false, new ms.settingHidden()));
	
	// Allow multiple attachees on the super vertex.
	this.addProperty(new ms.settingProperty('Multiple Attachees', false, new ms.settingHidden()));
	
	// Whether a rectangle ground object should be placed and not modified.
	this.addProperty(new ms.settingProperty('Grounded', true, new ms.settingBoolean(), null, true));
	
	// The penalty for winding past 360 degrees in terms of number of endpoints.
	// This affects the order in which the shapes are generated.
	this.addProperty(new ms.settingProperty('Winding Penalty', 1, new ms.settingHidden()));
	
	// The penalty for winding past 360 degrees when you are not following the preffered path 
	// in terms of number of endpoints.
	// This affects the order in which the shapes are generated.
	this.addProperty(new ms.settingProperty('Winding Penalty New', 10, new ms.settingHidden()));

	// Should we generate a new seed on the next synthesis operation.
	this.addProperty(new ms.settingProperty('New Seed', false, new ms.settingBoolean()));

	// The seed in the random number generator.
	this.addProperty(new ms.settingProperty('Seed', 1, new ms.settingNumber()));

	// The task count at which to stop.
	this.addProperty(new ms.settingProperty('Task Stop', -1, new ms.settingNumber(), null, true));

	// How many tasks to perform before pausing.
	this.addProperty(new ms.settingProperty('Task Step', 1, new ms.settingHidden()));

	// The task count at which to stop.
	this.addProperty(new ms.settingProperty('Debug Each Task', false, new ms.settingHidden()));

	this.addProperty(new ms.settingProperty('Fast Matrix Math', true, new ms.settingHidden()));

	this.addProperty(new ms.settingProperty('Report Timings', true, new ms.settingHidden()));

	// If the grid, numbers, and face connections should be shown.
	this.addProperty(new ms.settingProperty('Show Grid', false, new ms.settingHidden()));

	// Turns on high image smoothing quality.
	this.addProperty(new ms.settingProperty('High Smoothing', true, new ms.settingHidden()));

	// Use the WebGL version for grid view.
	this.addProperty(new ms.settingProperty('Use WebGL', false, new ms.settingBoolean(), null, true));

	// Always show faces.
	this.addProperty(new ms.settingProperty('Show Faces', true, new ms.settingHidden()));

	// If the holes should be cut into the faces.
	this.addProperty(new ms.settingProperty('Cut Holes', true, new ms.settingHidden()));

	// Forces there to be fewer start transitions.
	this.addProperty(new ms.settingProperty('Fewer Start Transitions', false, new ms.settingBoolean(), null, true));

	// Go into example view mode.
	this.addProperty(new ms.settingProperty('Example View', true, new ms.settingBoolean(), null, true));

	// Use the new network way for doing things.
	// This is now always true if the model is 3D, otherwise false.
	this.addProperty(new ms.settingProperty('Use Network', true, new ms.settingBoolean()));

	// Pause during a test scenario when finished with a particular shape.
	this.addProperty(new ms.settingProperty('Pause when finished', false, new ms.settingBoolean()));
};

ms.settings.prototype.addProperty = function(property) {
	this.properties[property.getLabel()] = property;
};

ms.settings.prototype.draw = function(element) {
	for (var key in this.properties) {
		this.properties[key].draw(element);
	}
};

ms.settings.prototype.report = function() {
	var report = '';
	for (var key in this.properties) {
		report += key + ': ' + this.properties[key].getValue() + '\n';
	}
	return report;
};

ms.settingProperty = function(label, value, type, opt_onChange, opt_hideForMvp) {
	this.value = value;
	this.label = label;
	this.type = (opt_hideForMvp && ms.mvp) ? new ms.settingHidden() : type;
	this.onChange = opt_onChange || null;

	// Another property that is linked so that when this property changes, it changes too.
	this.attachment = null;
};

ms.settingProperty.prototype.getValue = function() {
	return this.value;
};

ms.settingProperty.prototype.getLabel = function() {
	return this.label;
};

ms.settingProperty.prototype.setValue = function(value) {
	this.value = value;
	this.type.update(value);
	if (this.attachment) {
		this.attachment.setValue(value);
	}
	if (this.onChange) {
		this.onChange(value);
	}
};

ms.settingProperty.prototype.draw = function(element) {
	this.type.draw(this, element);
};

ms.settingProperty.prototype.copy = function() {
	return new ms.settingProperty(this.label, this.value, this.type.copy());
};

ms.settingProperty.prototype.attach = function(attachment) {
	this.attachment = attachment;
};

ms.settingProperty.prototype.deattach = function() {
	this.attachment = null;
};

ms.settingSelection = function(label, optionLabels, optionValues, defaultValue) {
	this.label = label;
	this.value = defaultValue;
	this.optionLabels = optionLabels;
	this.optionValues = optionValues;
	this.select = null;
};

ms.settingSelection.prototype.getValue = function() {
	return this.value;
};

ms.settingSelection.prototype.getLabel = function() {
	return this.label;
};

ms.settingSelection.prototype.draw = function(property, container) {
	var line = document.createElement('div');
	
	var labelElement = document.createElement('label');
	labelElement.innerHTML = this.label;
	line.appendChild(labelElement);
	
	this.select = document.createElement('select');
	this.select.id = this.label;
	for (var i = 0; i < this.optionLabels.length; i++) {
		var option = document.createElement('option');
		option.innerHTML = this.optionLabels[i];
		option.value = this.optionValues[i];
		this.select.appendChild(option);
	}
	this.select.value = this.value;
	this.select.addEventListener('change', this.onchange.bind(this, property));
	line.appendChild(this.select);
	container.appendChild(line);
};

ms.settingSelection.prototype.onchange = function(property, event) {
	property.setValue(event.target.value);
};

ms.settingSelection.prototype.update = function(value) {
	if (this.select) {
		this.select.value = value;
	}
};

ms.settingSelection.prototype.copy = function() {
	return new ms.settingSelection(this.label, this.optionLabels, this.optionValues, this.value);
};

ms.settingBoolean = function() {
	this.box = null;
};

ms.settingBoolean.prototype.copy = function() {
	return new ms.settingBoolean();
};

ms.settingBoolean.prototype.draw = function(property, container) {
	var line = document.createElement('div');
	line.className='boolean-setting';
	this.box = document.createElement('input');
	this.box.type = 'checkbox';
	this.box.checked = property.value;
	this.box.addEventListener('change', this.onchange.bind(this, property));
	line.appendChild(this.box);
	var labelElement = document.createElement('label');
	labelElement.innerHTML = property.label;
	line.appendChild(labelElement);
	container.appendChild(line);
};

ms.settingBoolean.prototype.onchange = function(property, event) {
	property.setValue(event.target.checked);
};

ms.settingBoolean.prototype.update = function(value) {
	if (this.box) {
		this.box.checked = value;
	}
};

ms.settingExtent = function() {};

ms.settingExtent.prototype.copy = function() {
	return new ms.settingExtent();
};

ms.settingExtent.prototype.draw = function(property, container) {
	var line = document.createElement('div');
	line.className='extent-setting';
	var labelElement = document.createElement('label');
	labelElement.innerHTML = 'Output Size'; // property.label;
	line.appendChild(labelElement);
	for (var i = 0; i < 3; i++) {
		var box = document.createElement('input');
		box.value = property.value[i];
		box.addEventListener('change', this.onchange.bind(this, property, i));
		line.appendChild(box);
	}
	container.appendChild(line);
};

ms.settingExtent.prototype.onchange = function(property, dim, event) {
	var extents = property.getValue();
	extents[dim] = parseInt(event.target.value);
};

ms.settingExtent.prototype.update = function(value) {};

ms.settingNumber = function(isString) {
	this.inputBox = null;
	this.isString = isString;
};

ms.settingNumber.prototype.copy = function() {
	return new ms.settingNumber(this.isString);
};

ms.settingNumber.prototype.draw = function(property, container) {
	var line = document.createElement('div');
	line.className = this.isString ? 'string-setting' : 'number-setting';
	var labelElement = document.createElement('label');
	labelElement.innerHTML = property.label;
	line.appendChild(labelElement);
	var box = document.createElement('input');
	//box.type = 'number';
	box.value = property.value;
	box.addEventListener('change', this.onchange.bind(this, property));
	this.inputBox = box;
	line.appendChild(box);
	container.appendChild(line);
};

ms.settingNumber.prototype.onchange = function(property, event) {
	var text = event.target.value;
	property.setValue(this.isString ? text : parseFloat(text));
};

ms.settingNumber.prototype.update = function(value) {
	if (this.inputBox) {
		this.inputBox.value = value;
	}
};

ms.settingHidden = function() {};

ms.settingHidden.prototype.copy = function() {
	return new ms.settingHidden();
};

ms.settingHidden.prototype.draw = function(label, value, element) {};

ms.settingHidden.prototype.update = function(value) {};

ms.settings.prototype.get = function(label) {
	return this.properties[label].getValue();
};

ms.settings.prototype.hasProperty = function(label) {
	return this.properties.hasOwnProperty(label)
};

ms.settings.prototype.set = function(label, value) {
	var prop = this.properties[label];
	return prop && prop.setValue(value);
};


ms.globalSettings = new ms.settings();

ms.globalSettings.export = function() {
	var result = {};
	Object.entries(ms.globalSettings.properties).forEach(function(property) { result[property[0]] = property[1].getValue(); });
	return JSON.stringify(result);
};

ms.globalSettings.import = function(newSettings) {
	var value = JSON.parse(newSettings);
	Object.entries(value).forEach(function(entry) {
		ms.globalSettings.set(entry[0], entry[1]);
	});
};

ms.globalSettings.default = ms.globalSettings.export();
