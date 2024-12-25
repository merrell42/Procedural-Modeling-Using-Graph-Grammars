ms.shapeView = function(canvas) {
	this.canvas = canvas;
	this.context = canvas.getContext("2d");
	
	var w = ms.mainController.getCanvasWidth();
	var h = ms.mainController.getCanvasHeight();
	this.viewport = new ms.viewport(1, w / 2, h / 2);
	this.controllerType = ms.mainController.controllerTypes.OBJECT;
	
	this.tmpVec1 = new ms.vec2(0, 0);
	this.tmpVec2 = new ms.vec2(0, 0);
	this.tmpVec3 = new ms.vec2(0, 0);
	this.tmpVec4 = new ms.vec2(0, 0);
};

ms.shapeView.OFFSET_X = -8;
ms.shapeView.OFFSET_Y = -8;

ms.shapeView.CONTROL_POINT_COLOR = { stroke: '#871', fill: '#ffb'};
ms.shapeView.HOVERED_CONTROL_COLOR = { stroke: '#521', fill: '#fc8'};
ms.shapeView.DEFAULT_COLOR = { stroke: '#055', fill: 'cyan'};
ms.shapeView.HOVERED_COLOR = { stroke: '#350', fill: '#8f0'};
ms.shapeView.SELECTED_COLOR = { stroke: '#541', fill: '#ff8'};
ms.shapeView.ARROW_ANGLE = 30 * Math.PI / 180;
ms.shapeView.ARROW_LENGTH = 15;

ms.shapeView.prototype.resize = function(width, height) {
	this.canvas.width = width;
	this.canvas.height = height;
};

ms.shapeView.prototype.getContext = function() {
	return this.context;
};

ms.shapeView.prototype.activate = function(controllerType) {
	this.canvas.style.display = '';
	this.canvas.style.position = '';	
	this.controllerType = controllerType;
};

ms.shapeView.prototype.activateOver = function() {
	this.canvas.style.display = '';
	this.canvas.style.position = 'absolute';
};

ms.shapeView.prototype.deactivate = function() {
	this.canvas.style.display = 'none';
	this.canvas.style.position = '';
	this.controllerType = -1;
};

ms.shapeView.prototype.getViewport = function(viewport) {
	return this.viewport;
};

ms.shapeView.prototype.drawLine = function(color, startPos, endPos, offset, options) {
	var hasArrows = options.hasArrows || [false, false];
	var arrowSize = options.arrowSize || ms.shapeView.ARROW_LENGTH;
	var brush = options.brush;
	var isArea = options.isArea || false;
	var optionsOffset = this.tmpVec3;
	if (options.offset) {
		optionsOffset = options.offset;
	} else {
		optionsOffset.set2(0, 0);
	}

	this.context.strokeStyle = color;
	this.context.beginPath();
	var sTransform = this.tmpVec1;
	var eTransform = this.tmpVec2;
	this.viewport.transform(startPos, offset, sTransform);
	this.viewport.transform(endPos,   offset, eTransform);
	sTransform.x += ms.shapeView.OFFSET_X;
	sTransform.y += ms.shapeView.OFFSET_Y;
	eTransform.x += ms.shapeView.OFFSET_X;
	eTransform.y += ms.shapeView.OFFSET_Y;
	// The offset is applied after the transformation.
	sTransform.add(optionsOffset);
	eTransform.add(optionsOffset);
	
	var tile = !options.secondPass && brush && brush.get('Image');
	if (!isArea && tile && tile != 'None') {
		if (!ms.globalSettings.get('Example View')) {
			this.context.globalAlpha = 0.5;
		}
		var image = ms.decorationView.getImage(tile);
		var angle = ms.vec2.angle(sTransform, eTransform);
		var scale = this.viewport.scale * ms.globalSettings.get('Major Grid Spacing');
		ms.lineStateCoordinates.drawEdgeTiles(this.context, tile, brush, angle, scale, sTransform, eTransform, []);
		this.context.globalAlpha = 1;
	}

	if (!isArea || options.isFirst) {
		this.context.moveTo(sTransform.x, sTransform.y);
	}
	// For the comparison drawing without any markings.
	this.context.lineTo(eTransform.x, eTransform.y);
	this.context.stroke();
	if (hasArrows[0]) {
		this.drawArrowHelper(startPos, endPos, offset, arrowSize);
	}
	if (hasArrows[1]) {
		this.drawArrowHelper(endPos, startPos, offset, arrowSize);
	}
};

ms.shapeView.prototype.drawArrowHelper = function(startPos, endPos, offset, size, opt_color) {
	var theta = ms.vec2.angle(startPos, endPos);
	var sTransform = startPos;
	var eTransform = ms.vec2.unitVec(theta).scale(size).add(sTransform);
	this.drawArrow(sTransform, eTransform, opt_color);
};

ms.shapeView.prototype.drawArrow = function(startPos0, endPos0, opt_color) {
	if (opt_color) {
		this.context.strokeStyle = opt_color;
	}
	this.context.lineWidth = 1;
	this.context.beginPath();
	var startPos = new ms.vec2(0, 0);
	var endPos = new ms.vec2(0, 0);
	this.viewport.transform(startPos0, ms.vec2.ORIGIN, startPos);
	this.viewport.transform(endPos0, ms.vec2.ORIGIN, endPos);
	
	var angle = ms.vec2.angle(startPos, endPos);
	var size = startPos.distance(endPos);

	var phi = angle + ms.shapeView.ARROW_ANGLE;
	var dx = size * Math.cos(phi);
	var dy = size * Math.sin(phi);
	this.context.moveTo(startPos.x + ms.shapeView.OFFSET_X,      startPos.y + ms.shapeView.OFFSET_Y);
	this.context.lineTo(startPos.x + ms.shapeView.OFFSET_X + dx, startPos.y + ms.shapeView.OFFSET_Y + dy);
	this.context.moveTo(startPos.x + ms.shapeView.OFFSET_X,      startPos.y + ms.shapeView.OFFSET_Y);
	phi = angle - ms.shapeView.ARROW_ANGLE;
	var dx = size * Math.cos(phi);
	var dy = size * Math.sin(phi);
	this.context.lineTo(startPos.x + ms.shapeView.OFFSET_X + dx, startPos.y + ms.shapeView.OFFSET_Y + dy);
	this.context.stroke();
};

ms.shapeView.prototype.drawGrids = function(offset) {
	var width = this.canvas.width;
	var height = this.canvas.height;
	var spacing = ms.globalSettings.get('Snap Grid Spacing');
	var majorSpacing = ms.globalSettings.get('Major Grid Spacing');
	var x0 = ms.shapeView.OFFSET_X;
	var y0 = ms.shapeView.OFFSET_Y;
	
	var t1 = this.tmpVec1;
	var t2 = this.tmpVec2;
	var c1 = this.tmpVec3;
	var c2 = this.tmpVec4;
	for (var x = 0; x < width; x += spacing) {
		if (x % majorSpacing == 0) {
			this.context.strokeStyle = '#ddd';
			this.context.lineWidth = 1;
		} else {
			this.context.strokeStyle = '#e2e2e2';
			this.context.lineWidth = 0.5;
		}
		this.context.beginPath();
		c1.set2(x, 0);
		c2.set2(x, height);
		this.viewport.transform(c1, offset, t1);
		this.viewport.transform(c2, offset, t2);
		this.context.moveTo(t1.x + x0, t1.y + y0);
		this.context.lineTo(t2.x + x0, t2.y + y0);
		this.context.closePath();
		this.context.stroke();
	}
	for (var y = 0; y < height; y += spacing) {
		if (y % majorSpacing == 0) {
			this.context.strokeStyle = '#ddd';
			this.context.lineWidth = 1;
		} else {
			this.context.strokeStyle = '#e2e2e2';
			this.context.lineWidth = 0.5;
		}
		this.context.beginPath();
		c1.set2(0, y);
		c2.set2(width, y);
		this.viewport.transform(c1, offset, t1);
		this.viewport.transform(c2, offset, t2);
		this.context.moveTo(t1.x + x0, t1.y + y0);
		this.context.lineTo(t2.x + x0, t2.y + y0);
		this.context.closePath();
		this.context.stroke();
	}
};

ms.shapeView.prototype.redraw = function (driver) {
	this.context.imageSmoothingQuality = ms.globalSettings.get('High Smoothing') ? 'high' : 'low';
	this.context.globalAlpha = 1;
	this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
	if (ms.highlightedElement) {
		ms.highlightedElement.highlight(this);
	}
	var renderables = driver.getRenderables();
	var offset = ms.vec2.ORIGIN;
	if (ms.globalSettings.get('Snap Grid') && this.controllerType != ms.mainController.controllerTypes.GRAPH) {
		this.drawGrids(offset);
	}
	for (var i = 0; i < renderables.length; i++) {
		renderables[i].draw(this, offset);
	}
};

ms.shapeView.prototype.drawVertex = function(v, selected, secondPass) {
	var offset = ms.vec2.ORIGIN;
	var decoration = v.getDecoration();
	var hasImage = !secondPass && decoration && decoration.hasImage();
	var spacing = ms.globalSettings.get('Major Grid Spacing');
	if ((selected && v.getVisible()) || hasImage) {
		var context = this.context;
		var transformed = this.tmpVec1;
		var centerX = decoration ? decoration.get('Center X') : 0;
		var centerY = decoration ? decoration.get('Center Y') : 0;
		var pos = v.getPosition().copy();
		this.viewport.transform(pos, offset, transformed);
		var center = {
			x: transformed.x + ms.shapeView.OFFSET_X,
			y: transformed.y + ms.shapeView.OFFSET_Y
		};
		if (hasImage) {
			pos.move(spacing * centerX, spacing * centerY);
			this.viewport.transform(pos, offset, transformed);
			var imageCenter = {
				x: transformed.x + ms.shapeView.OFFSET_X,
				y: transformed.y + ms.shapeView.OFFSET_Y
			};
			
			
			// context.globalAlpha = 0.5;
			var imageName = decoration.get('Image');
			var image = ms.decorationView.getImage(imageName);
			var scale = this.viewport.scale * spacing;
			context.save();
			context.translate(imageCenter.x, imageCenter.y);
			// context.rotate(angle);
			var w = scale * decoration.get('Width');
			var h = scale * decoration.get('Height');
			context.translate(-w / 2, -h / 2);
			context.drawImage(image, 0, 0, w, h);
			context.restore();
			context.globalAlpha = 1;
		}
		
		var color = v.getColor();
		var isLarge = v.isLarge ? v.isLarge() : true;
		context.fillStyle = color.fill;
		context.strokeStyle = color.stroke;
		context.beginPath();
		context.lineWidth = isLarge ? 4 : 2;
		var radius = isLarge ? 7 : 4;
		var transformed = this.tmpVec1;
		this.viewport.transform(v.getPosition(), offset, transformed);
		context.arc(center.x, center.y, radius, 0, 2 * Math.PI, true);
		context.closePath();
		context.stroke();
		context.fill();
	}
};

ms.shapeView.prototype.drawPoint = function(position0, color) {
	var transformed = this.tmpVec1;
	var position = position0.copy();
	this.swapY(position);
	this.viewport.transform(position, ms.vec2.ORIGIN, transformed);
	var context = this.context;
	context.beginPath();
	context.arc(transformed.x + ms.shapeView.OFFSET_X, transformed.y + ms.shapeView.OFFSET_Y, 2, 0, 2 * Math.PI, true);
	context.closePath();
	context.strokeStyle = color;
	context.stroke();
	context.fillStyle = color;
	context.fill();
};

ms.shapeView.prototype.swapY = function(vec) {
	vec.y = this.canvas.height - vec.y;
};

ms.shapeView.prototype.drawEdge = function(startPos0, endPos0, color, offsetScale, lineWidth, options) {
	var context = this.getContext();
	context.lineWidth = lineWidth;
	var startPos = startPos0.copy();
	var endPos = endPos0.copy();
	this.swapY(startPos);
	this.swapY(endPos);
	
	var dir = endPos.copy().minus(startPos);
	dir.normalize();
	var offset = new ms.vec2(-dir.y, dir.x);
	offset.scale(offsetScale * ms.graph.FACE_EDGE_WIDTH / 3);

	context.beginPath();
	var lineOptions = { hasArrows: [false, false], offset: offset };
	this.drawLine(color, startPos, endPos, ms.vec2.ORIGIN, lineOptions);
	context.stroke();
};

ms.shapeView.prototype.drawCircleOld = function(position, offset, r, opt_options) {
	var filled = !opt_options || opt_options.fill;
	
	var radius = r * this.viewport.scale;
	this.context.beginPath();
	if (filled) {
		this.context.lineWidth = 2;
	}
	var transformed = this.tmpVec1;
	this.viewport.transform(position, offset, transformed);
	var startAngle = opt_options && opt_options.hasOwnProperty('startAngle') ? opt_options.startAngle : 0;
	var endAngle = opt_options && opt_options.hasOwnProperty('endAngle') ? opt_options.endAngle : 2 * Math.PI;
	this.context.arc(transformed.x + ms.shapeView.OFFSET_X, transformed.y + ms.shapeView.OFFSET_Y, radius, startAngle, endAngle, true);
	this.context.closePath();
	filled && this.context.fill();
	this.context.stroke();
};

ms.shapeView.prototype.drawCircle = function(v0, r0, r1, color) {
	// TODO: Put in the right color.
	this.context.strokeStyle = '#000';
	
	this.context.lineWidth = 1;
	var radius = r0 * this.viewport.scale;
	this.context.beginPath();
	var position = v0.copy();
	this.swapY(position);
	var transformed = this.tmpVec1;
	this.viewport.transform(position, ms.vec3.ORIGIN, transformed);
	var startAngle = 0;
	var endAngle = 2 * Math.PI;
	this.context.arc(transformed.x + ms.shapeView.OFFSET_X, transformed.y + ms.shapeView.OFFSET_Y, radius, startAngle, endAngle, true);
	this.context.closePath();
	this.context.stroke();
};

ms.shapeView.prototype.drawText = function(position, text, color) {
	var transformed = this.tmpVec1;
	this.viewport.transform(position, ms.vec2.ORIGIN, transformed);
	var context = this.context;
	context.fillStyle = color;
	context.fillText(text, transformed.x + ms.shapeView.OFFSET_X, transformed.y + ms.shapeView.OFFSET_Y);
	context.fill();
};

ms.shapeView.prototype.convertToScreen = function(position, color) {
	var transformed = this.tmpVec1;
	this.viewport.transform(position, ms.vec2.ORIGIN, transformed);
	return transformed;
};

ms.shapeView.prototype.drawQuad = function(corners, color) {};