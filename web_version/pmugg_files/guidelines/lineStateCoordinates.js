ms.lineStateCoordinates = function(angledEdges, scale, opt_length) {
	this.angledEdges = angledEdges;
	this.scale = scale;
	
	this.imageSeeds = [];
	for (var i = 0; i < ms.lineStateCoordinates.numImageSeeds; i++) {
		this.imageSeeds.push(ms.random(100));
	}

	this.dirtyRenderData = true;
	this.renderData = null;
	if (opt_length) {
		this.length = opt_length;
		this.dirtyLength = false;
	} else {
		this.length = 0;
		this.dirtyLength = true;
	}
};

ms.lineStateCoordinates.numImageSeeds = 20;

ms.lineStateCoordinates.prototype.copy = function() {
	var angledEdges = this.angledEdges.map(function(angleEdge) {
		return angleEdge.copy();
	});
	return new ms.lineStateCoordinates(angledEdges, this.scale, this.length);
};

ms.lineStateCoordinates.prototype.getAngle = function() {
	return this.angledEdges[0].getAngle();
};

ms.lineStateCoordinates.prototype.getScale = function() {
	return this.scale;
};

ms.lineStateCoordinates.prototype.getAngledEdges = function() {
	return this.angledEdges;
};

ms.lineStateCoordinates.prototype.getLength = function() {
	if (this.dirtyLength) {
		var p0 = this.angledEdges[0].getPosition();
		var p1 = this.angledEdges[1].getPosition();
		this.length = p0.distance(p1);
		this.dirtyLength = false;
	}
	return this.length;
};

ms.lineStateCoordinates.prototype.intersect = function(coordinatesB, opt_thickness1, opt_thickness2) {	
	var thickness1 = (opt_thickness1 === undefined) ? ms.intersector.THICKNESS : opt_thickness1;
	var thickness2 = (opt_thickness2 === undefined) ? opt_thickness1 : opt_thickness2;
	var angledEdgesB = coordinatesB.getAngledEdges();
	options = {thickness2: opt_thickness2, parallelIntersect: true};
	return ms.intersector.intersect(
		this.angledEdges[0].getPosition(),
		this.angledEdges[1].getPosition(),
		angledEdgesB[0].getPosition(),
		angledEdgesB[1].getPosition(),
		thickness1,
		options);
};

/* ms.lineStateCoordinates.DEFAULT_TEXCOORDS = [
	0.05, 0.05,
	0.05, 1,
	1, 0.05,
	1, 0.05,
	0.05, 1,
	1, 1,
]; */

ms.lineStateCoordinates.DEFAULT_TEXCOORDS = [
	0.0, 0.0,
	0.0, 1,
	1, 0.0,
	1, 0.0,
	0.0, 1,
	1, 1,
];

ms.lineStateCoordinates.DEFAULT_WIDTH = 0.05;

ms.lineStateCoordinates.RENDER_TYPE = {
	LINE: 0,
	CONVEX: 1,
	CONCAVE: 2,
};

ms.lineStateCoordinates.getRenderDataOld = function(brush, seg0, seg3, opt_renderType) {
	var RENDER_TYPE = ms.lineStateCoordinates.RENDER_TYPE;
	var renderType = opt_renderType || RENDER_TYPE.LINE;
	
	var p0 = seg0.getPosition();
	var p3 = seg3.getPosition();
	var t0 = seg0.getT();
	var t3 = seg3.getT();
	
	var u = p3.copy().minus(p0);
	
	var v0 = ms.vec2.unitVec(seg0.getAngle() - Math.PI / 2);
	var v3 = ms.vec2.unitVec(seg3.getAngle() + Math.PI / 2);
	
	var U = mathG.matrix([
		[ v0.x, -v3.x],
		[ v0.y, -v3.y],
	]);
	var det = mathG.det(U);
	var sign = 1;
	var sPrime = Infinity;
	var intersection = null;
	if (Math.abs(det) > 1e-6) {
		var v = new ms.vec2(-u.y, u.x);
		v.normalize();
		if (v.dot(v0) < 0) {
			v.scale(-1);
		}
		if (det < 0) {
			sign = -1;
		}
		var T = mathG.multiply(mathG.inv(U), mathG.matrix([[u.x], [u.y]]));
		intersection = v0.copy().scale(T.get([0, 0]));
		sPrime = v.dot(intersection);
		intersection.add(p0);
	}
	
	var length = u.length();
	var numTiles = 1;
	var tiled = brush ? brush.get('Tiled') : true;
	if (!tiled) {
		debugger;
	}
	if (tiled && brush) {
		var lengthInTiles = length / brush.get('Tile Length');
		numTiles = Math.max(1, Math.round(lengthInTiles));
	}

	var offset = brush ? brush.get('Offset') : 0;
	// TODO: Find the aspect ratio of the image to have non-tiled images work.
	var width = tiled ?
		(brush ? brush.get('Width') : ms.lineStateCoordinates.DEFAULT_WIDTH) :
		image.height / image.width * length;
	
	var s0 = -offset - width / 2;
	var s1 = -offset + width / 2;
	var renderData = {texcoords: [], positions: []};
	if (renderType == RENDER_TYPE.CONVEX) {
		var temp = s0;
		s0 = s1;
		s1 = temp;
		s0 *= -1;
		s1 *= -1;
	}
	
	var p00 = null;
	var p01 = null;
	var p30 = null;
	var p31 = null;
	var texCoords = ms.lineStateCoordinates.DEFAULT_TEXCOORDS;
	var clipped = false
	if (sign != 1) {
		if (s1 < sPrime) {
			return renderData;
		} else if (s0 < sPrime) {
			clipped = true;
		}
	} else {
		if (s0 > sPrime) {
			return renderData;
		} else if (s1 > sPrime) {
			clipped = true;
		}
	}
	
	if (clipped) {
		var tPrime = (sPrime - s0) / (s1 - s0);
		texCoords = texCoords.slice();
		if ((sign == 1) && renderType == RENDER_TYPE.LINE) {
			p01 = intersection;
			p31 = intersection;
			texCoords[3] = tPrime;
			texCoords[9] = tPrime;
			texCoords[11] = tPrime;
		} else {
			p00 = intersection;
			p30 = intersection;
			if (renderType == RENDER_TYPE.CONVEX) {
				texCoords[1] = 1 - tPrime;
				texCoords[5] = 1 - tPrime;
				texCoords[7] = 1 - tPrime;
				texCoords[3] = 0;
				texCoords[9] = 0;
				texCoords[11] = 0;
			} else {
				texCoords[1] = tPrime;
				texCoords[5] = tPrime;
				texCoords[7] = tPrime;
			}
		}
	}

	p00 = p00 || p0.copy().add(v0.copy().scale(s0));
	p01 = p01 || p0.copy().add(v0.copy().scale(s1));
	p30 = p30 || p3.copy().add(v3.copy().scale(s0));
	p31 = p31 || p3.copy().add(v3.copy().scale(s1));
	
	for (var i = 0; i < numTiles; i++) {
		ms.util.fastConcat(renderData.texcoords, texCoords);
		var s0 = i / numTiles;
		var s1 = (i + 1) / numTiles;
		var q00 = ms.vec2.lerp(p00, p30, s0);
		var q01 = ms.vec2.lerp(p01, p31, s0);
		var q10 = ms.vec2.lerp(p00, p30, s1);
		var q11 = ms.vec2.lerp(p01, p31, s1);
		var newPositions = [
			q00.x, q00.y,
			q01.x, q01.y,
			q10.x, q10.y,
			q10.x, q10.y,
			q01.x, q01.y,
			q11.x, q11.y
		];
		ms.util.fastConcat(renderData.positions, newPositions);
	}
	return renderData;
};

ms.lineStateCoordinates.getTileImage = function(images, tileSeeds, tileNum) {	
	var imageNum = ms.util.consistentRandom(images.length, tileSeeds.base + tileNum * tileSeeds.increment);
	return images[imageNum];
};

ms.lineStateCoordinates.getRenderData = function(renderData, brush, seg0, seg3, tileSeeds, opt_renderType) {
	var RENDER_TYPE = ms.lineStateCoordinates.RENDER_TYPE;
	var renderType = opt_renderType || RENDER_TYPE.LINE;
	
	var p0 = seg0.getPosition();
	var p3 = seg3.getPosition();
	var t0 = seg0.getT();
	var t1 = seg3.getT();
	var reversed = false;
	if (t1 < t0) {
		reversed = true;
		var temp = t0;
		t0 = t1;
		t1 = temp;
	}
	if (renderType != RENDER_TYPE.LINE) {
		t0 = 0;
		t1 = 1;
		reversed = false;
	}
	
	var u = p3.copy().minus(p0);
	
	var v0 = ms.vec2.unitVec(seg0.getAngle() - Math.PI / 2);
	var v3 = ms.vec2.unitVec(seg3.getAngle() + Math.PI / 2);
	
	var U = mathG.matrix([
		[ v0.x, -v3.x],
		[ v0.y, -v3.y],
	]);
	var det = mathG.det(U);
	var sign = 1;
	var sPrime = Infinity;
	var intersection = null;
	if (Math.abs(det) > 1e-6) {
		var v = new ms.vec2(-u.y, u.x);
		v.normalize();
		if (v.dot(v0) < 0) {
			v.scale(-1);
		}
		if (det < 0) {
			sign = -1;
		}
		var T = mathG.multiply(mathG.inv(U), mathG.matrix([[u.x], [u.y]]));
		intersection = v0.copy().scale(T.get([0, 0]));
		sPrime = v.dot(intersection);
		intersection.add(p0);
	}
	
	var length = u.length();
	var tiled = brush ? brush.get('Tiled') : true;
	var numTiles = Math.ceil(t1) - Math.floor(t0);

	var offset = brush ? brush.get('Offset') : 0;
	// TODO: Find the aspect ratio of the image to have non-tiled images work.
	var width = tiled ?
		(brush ? brush.get('Width') : ms.lineStateCoordinates.DEFAULT_WIDTH) :
		image.height / image.width * length;
	
	var s0 = -offset - width / 2;
	var s1 = -offset + width / 2;
	if (renderType == RENDER_TYPE.CONVEX) {
		var temp = s0;
		s0 = s1;
		s1 = temp;
		s0 *= -1;
		s1 *= -1;
	}
	
	var p00 = null;
	var p01 = null;
	var p30 = null;
	var p31 = null;
	var texCoords = ms.lineStateCoordinates.DEFAULT_TEXCOORDS;
	var clipped = false
	if (sign != 1) {
		if (s1 < sPrime) {
			return;
		} else if (s0 < sPrime) {
			clipped = true;
		}
	} else {
		if (s0 > sPrime) {
			return;
		} else if (s1 > sPrime) {
			clipped = true;
		}
	}
	
	if (clipped) {
		var tPrime = (sPrime - s0) / (s1 - s0);
		texCoords = texCoords.slice();
		if ((sign == 1) && renderType == RENDER_TYPE.LINE) {
			p01 = intersection;
			p31 = intersection;
			texCoords[3] = tPrime;
			texCoords[9] = tPrime;
			texCoords[11] = tPrime;
		} else {
			p00 = intersection;
			p30 = intersection;
			if (renderType == RENDER_TYPE.CONVEX) {
				texCoords[1] = 1 - tPrime;
				texCoords[5] = 1 - tPrime;
				texCoords[7] = 1 - tPrime;
				texCoords[3] = 0;
				texCoords[9] = 0;
				texCoords[11] = 0;
			} else {
				texCoords[1] = tPrime;
				texCoords[5] = tPrime;
				texCoords[7] = tPrime;
			}
		}
	}

	p00 = p00 || p0.copy().add(v0.copy().scale(s0));
	p01 = p01 || p0.copy().add(v0.copy().scale(s1));
	p30 = p30 || p3.copy().add(v3.copy().scale(s0));
	p31 = p31 || p3.copy().add(v3.copy().scale(s1));
	
	var images = brush && brush.get('Image');

	var step0 = t0;
	var step1 = Math.min(Math.ceil(step0 + 1e-6), t1);
	var tileCount = 0;
	while (step0 < t1) {
		var newRenderData;
		if (images && images != 'None') {
			var fileName = ms.lineStateCoordinates.getTileImage(images.split(','), tileSeeds, Math.floor(step0));
			newRenderData = renderData.getTextureData(fileName);
		} else {
			var color = brush ? brush.get('color') : '#880000';
			if (color && color != '#fff') {
				newRenderData = renderData.getColorData(color);
			}
		}
		tileCount++;

		texCoords = texCoords.slice();
		var r0 = step0 - Math.floor(step0);
		var r1 = step1 - Math.floor(step0);
		// var q0 = reversed ? r1 : r0;
		// var q1 = reversed ? r0 : r1;
		var q0 = r0;
		var q1 = r1;
		texCoords[0]  = q0;
		texCoords[2]  = q0;
		texCoords[4]  = q1;
		texCoords[6]  = q1;
		texCoords[8]  = q0;
		texCoords[10] = q1;
		ms.util.fastConcat(newRenderData.texcoords, texCoords);
		
		var s0,s1;
		if (!reversed) {
			s0 = (step0 - t0) / (t1 - t0);
			s1 = (step1 - t0) / (t1 - t0);
		} else {
			s0 = (step0 - t1) / (t0 - t1);
			s1 = (step1 - t1) / (t0 - t1);
		}
		var q00 = ms.vec2.lerp(p00, p30, s0);
		var q01 = ms.vec2.lerp(p01, p31, s0);
		var q10 = ms.vec2.lerp(p00, p30, s1);
		var q11 = ms.vec2.lerp(p01, p31, s1);
		var newPositions = [
			q00.x, q00.y,
			q01.x, q01.y,
			q10.x, q10.y,
			q10.x, q10.y,
			q01.x, q01.y,
			q11.x, q11.y
		];
		ms.util.fastConcat(newRenderData.positions, newPositions);

		step0 = step1;
		step1 = Math.min(Math.ceil(step0 + 1e-6), t1);
	}
};

ms.lineStateCoordinates.drawEdgeTiles = function(context, images, brush, angle, scale, p0, p3, imageSeeds) {
	var dx = p3.x - p0.x;
	var dy = p3.y - p0.y;
	var length = Math.sqrt(dx * dx + dy * dy);
	var numTiles = 1;
	var tiled = brush.get('Tiled');
	if (tiled) {
		var lengthInTiles = length / scale / brush.get('Tile Length');
		numTiles = Math.max(1, Math.round(lengthInTiles));
	}
	length /= numTiles;
	dx /= numTiles;
	dy /= numTiles;
	
	var p = JSON.parse(JSON.stringify(p0));
	var offset = brush.get('Offset');
	if (offset) {
		var v = new ms.vec2(-dy, dx);
		v.normalize().scale(-offset * scale);
		p.x += v.x;
		p.y += v.y;
	}
	var numSeeds = ms.lineStateCoordinates.numImageSeeds
	for (var i = 0; i < numTiles; i++) {
		var imageSeed = (imageSeeds[i % numSeeds] + 17 * Math.floor(i / numSeeds)) % 100;
		var image = ms.decorationView.getImage(images, imageSeed);
		context.save();
		context.translate(p.x, p.y);
		context.rotate(angle);
		var width = tiled ?
			brush.get('Width') * scale :
			image.height / image.width * length;
		context.translate(0, -width / 2);
		// Not sure about length + 1 instead of length.
		context.drawImage(image, 0, 0, length + 1, width);
		context.restore();
		p.x += dx;
		p.y += dy;
	}
};

ms.lineStateCoordinates.fastConcat = function(allData, newData) {
	ms.util.fastConcat(allData.texcoords, newData.texcoords);
	ms.util.fastConcat(allData.positions, newData.positions);
};

ms.lineStateCoordinates.prototype.fillRenderData = function(renderData, brush, tileSeeds) {
	if (brush && !brush.get('Visible')) {
		return;
	}

	var startSeg = this.angledEdges[0];
	var endSeg = this.angledEdges[1];
	if (brush && brush.get('Flip')) {
		var temp = startSeg;
		startSeg = endSeg;
		endSeg = temp;
	}
	ms.lineStateCoordinates.getRenderData(renderData, brush, startSeg, endSeg, tileSeeds);

	/* var tile = brush && brush.get('Image');
	var allData = null;
	if (tile && tile != 'None') {
		allData = renderData.getTextureData(tile);
	} else {
		var color = brush ? brush.get('color') : '#880000';
		if (color && color != '#fff') {
			allData = renderData.getColorData(color);
		}
	}
	if (allData) {
		if (this.dirtyRenderData) {
			this.renderData = ms.lineStateCoordinates.getRenderData(brush, startSeg, endSeg, this.imageSeeds);
			this.dirtyRenderData = false;
		}
		ms.lineStateCoordinates.fastConcat(allData, this.renderData);
	} */
};

ms.lineStateCoordinates.prototype.draw = function(context, isRigid, brush, convertToScreen) {
	var startSeg = this.angledEdges[0];
	var endSeg = this.angledEdges[1];
	var startPos = startSeg.getPosition().copy();
	var endPos = endSeg.getPosition().copy();
	if (brush && !brush.get('Visible')) {
		return;
	}
	if (brush && brush.get('Flip')) {
		var temp = startPos;
		startPos = endPos;
		endPos = temp;
	}

	var p0 = convertToScreen(startPos);
	var p3 = convertToScreen(endPos);

	var tile = brush && brush.get('Image');
	if (tile && tile != 'None') {
		var angle = -ms.vec2.angle(startPos, endPos);
		ms.lineStateCoordinates.drawEdgeTiles(context, tile, brush, angle, convertToScreen.scale, p0, p3, this.imageSeeds);
	} else {
		context.globalAlpha = 1;
		context.beginPath();
		context.moveTo(p0.x, p0.y);
		if (isRigid) {
			context.lineTo(p3.x, p3.y);
		} else {
			var startAngle = startSeg.getAngle();
			var endAngle = endSeg.getAngle();

			var d = startPos.distance(endPos);		
			var startDir = ms.vec2.unitVec(startAngle);
			var endDir = ms.vec2.unitVec(endAngle);
			var startControl = startDir.copy().scale(d / 3).add(startPos);
			var endControl = endDir.copy().scale(d / 3).add(endPos);
			
			var p1 = convertToScreen(startControl);
			var p2 = convertToScreen(endControl);
			context.bezierCurveTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
		}
		context.stroke();
	}
};

ms.lineStateCoordinates.prototype.getSplitPoint = function() {
	if (this.angledEdges.length != 2) {
		ms.alert('Only guide coordinates with two angledEdges should be split');
	}
	var position0 = this.angledEdges[0].getPosition();
	var position1 = this.angledEdges[1].getPosition();
	return ms.vec2.lerp(position0, position1, Math.random());
};

ms.lineStateCoordinates.prototype.split = function(point) {
	var angledEdgeEnd0 = new ms.angledEdge(point, this.angledEdges[1].getAngle(), 0);
	var angledEdgeStart1 = new ms.angledEdge(point, this.angledEdges[0].getAngle(), 0);

	var newAngledEdges0 = [this.angledEdges[0], angledEdgeEnd0];
	var newAngledEdges1 = [angledEdgeStart1, this.angledEdges[1]];
	return [
			new ms.lineStateCoordinates(newAngledEdges0, this.scale),
			new ms.lineStateCoordinates(newAngledEdges1, this.scale)
	];
};

ms.lineStateCoordinates.prototype.setPosition = function(point, isAtStart) {
	this.dirtyLength = true;
	this.dirtyRenderData = true;
	this.angledEdges[isAtStart ? 0 : 1].setPosition(point);
};

ms.lineStateCoordinates.prototype.copyWithAngledEdges = function(angledEdges) {
	return new ms.lineStateCoordinates(angledEdges, this.scale);
};

ms.lineStateCoordinates.prototype.print = function() {
	var result = '';
	for (var i = 0; i < this.angledEdges.length; i++) {
		result += this.angledEdges[i].getPosition().print() + ' ';
	}
	return result;
};