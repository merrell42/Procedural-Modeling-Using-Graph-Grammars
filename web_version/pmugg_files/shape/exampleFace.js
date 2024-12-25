ms.exampleFace = function(area) {
	this.area = area;
	this.endpoints = [];
	// A face that encloses this one.
	this.outerComponent = null;
	// The faces that this encloses.
	this.innerComponents = [];
	this.cachedArea = 0;
	this.dirty = true;
	this.tmpVec1 = new ms.vec2(0, 0);
};

ms.exampleFace.prototype.setDirty = function(dirty) {
	this.dirty = dirty;
};

ms.exampleFace.prototype.isDirty = function() {
	return this.dirty;
};

ms.exampleFace.prototype.selectType = function() {
	return ms.shapeMaker.SelectableTypes.FACE;
};

ms.exampleFace.prototype.getSelected = function() {
	return false;
};

ms.exampleFace.prototype.select = function() {};

ms.exampleFace.prototype.addInnerComponent = function(innerComponent) {
	this.innerComponents.push(innerComponent);
	innerComponent.outerComponent = this;
};

ms.exampleFace.prototype.addEndpoint = function(endpoint) {
	this.endpoints.push(endpoint);
	endpoint.setFace(this);
};

ms.exampleFace.prototype.addEndpoints = function(endpoint0) {
	this.addEndpoint(endpoint0);
	
	var endpoint = endpoint0.getNext().twin();
	while (endpoint != endpoint0) {
		this.addEndpoint(endpoint);
		endpoint = endpoint.getNext().twin();
	}
	this.dirty = false;
};

ms.exampleFace.prototype.getArea = function() {
	return this.area;
};

ms.exampleFace.prototype.setArea = function(area) {
	this.area = area;
};

ms.exampleFace.prototype.signedArea = function() {
	return ms.shapeSet.faceArea(this.endpoints);
};

ms.exampleFace.prototype.updateComponents = function(exampleShape) {
	if (!this.dirty) {
		return;
	}
	this.updateComponents0(exampleShape);
	this.dirty = false;
};

// Similar to ms.face.updateConnection.
ms.exampleFace.prototype.updateComponents0 = function(exampleShape) {
	this.cachedArea = this.signedArea();
	if (this.cachedArea <= 0) {
		this.outerComponent = null;
		return;
	}
	
	var pLeft = ms.face.leftmostEndpoint(this.endpoints).getPosition();
	var nearest = exampleShape.nearestLeftEdge(pLeft);
	if (!nearest.endpoint) {
		return;
	}
	var face = nearest.endpoint.getFace();
	face.updateComponents(exampleShape);
	if (face.cachedArea > 0) {
		if (face.outerComponent) {
			face.outerComponent.addInnerComponent(this);
		}
	} else {
		face.addInnerComponent(this);
	}
};

ms.exampleFace.getFillStyle = function(context, area, scale, getPosition, imageSeed) {
	if (!area) {
		return '';
	}
	var imageNames = area.get('Image');
	if (imageNames && imageNames != 'None') {
		var imageName = ms.decorationView.pickImage(imageNames, imageSeed);
		var image = ms.decorationView.getImage(imageName);
		var pattern = context.createPattern(image, 'repeat');
		if (!pattern) {
			// This is sometimes null when first initializing.
			return area.getColor();
		}
		var position = getPosition();
		var transform = svg1.createSVGMatrix()
			.translate(position.x, position.y)
			.scaleNonUniform(
				scale * area.get('Width') / image.width,
				scale * area.get('Height') / image.height);
		pattern.setTransform(transform);
		return pattern;
	} else {
		return area.getColor();
	}
};

ms.exampleFace.prototype.draw = function(view) {
	// The face is dirty before it is drawn.
	if (!this.dirty) {
		return;
	}
	this.dirty = false;
	// Always draw the outer component first.
	this.outerComponent && this.outerComponent.draw(view);
	// I think this is needed in some highly nested cases.
	for (var i = 0; i < this.endpoints.length; i++) {
		this.endpoints[i].twin().getFace().draw(view);
	}
	if (this.cachedArea >= 0) {
		return;
	}
	var fillStyle = '#fff';
	var spacing = ms.globalSettings.get('Major Grid Spacing');
	if (this.area) {
		var self = this;
		var getPosition = function() {
			var pLeft = ms.face.leftmostEndpoint(self.endpoints).getPosition();
			var transformed = self.tmpVec1;
			view.viewport.transform(pLeft, ms.vec2.ORIGIN, transformed);
			transformed.x += ms.shapeView.OFFSET_X;
			transformed.y += ms.shapeView.OFFSET_Y;
			return transformed;
		}
		var scale = view.viewport.scale * spacing;
		fillStyle = ms.exampleFace.getFillStyle(view.context, this.area, scale, getPosition);
		if (!ms.globalSettings.get('Example View')) {
			view.context.globalAlpha = 0.5;
		}
	}
	if (fillStyle == '#fff') {
		view.context.globalAlpha = 1;
		return;
	}
	view.context.fillStyle = fillStyle;
	view.context.beginPath();
	for (var i = 0; i < this.endpoints.length; i++) {
		var e = this.endpoints[i];
		e.getEdge().drawArea(view, ms.vec2.ORIGIN, {isArea: true, isFirst: i == 0, reverse: e.isAtStart});
	}	
	view.context.closePath();
	view.context.fill();
	view.context.globalAlpha = 1;
};