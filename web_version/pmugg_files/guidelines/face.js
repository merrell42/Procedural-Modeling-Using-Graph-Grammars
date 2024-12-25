ms.face = function(faceType, stats) {
	var properties = {
			// The endpoints defined the outer boundary of the face.
			endpoint: new ms.alternativeArray('endpoint', true),
			faceConnection: new ms.singleProperty('faceConnection', /* required */ false),
			faceGroup: new ms.singleProperty('faceGroup'),
			looped: new ms.valueProperty('looped'),
			hole: new ms.valueProperty('hole'),
			collection: new ms.singleProperty('collection'),
			bspPolygon: new ms.alternativeArray('bspPolygon', false),
	};
	this.node = new ms.node(this, stats, 'face', properties);
	this.setLooped(false);
	this.setHole(false);
	this.imageSeed = ms.random(100);
	this.faceType = faceType;
	this.dirty = true;
};

ms.face.prototype.getNode = function() {
	return this.node;
};

ms.face.prototype.createGroup = function() {
	var group = new ms.faceGroup(this.node.getStats());
	this.getNode().connect(group);
};

ms.face.prototype.splitGroup = function() {
	this.getGroup().getNode().disconnect(this);
	this.createGroup();
};

ms.face.prototype.getEndpoints = function() {
	return this.node.get('endpoint');
};

ms.face.prototype.lastEndpoint = function() {
	var endpoints = this.node.get('endpoint');
	return endpoints[endpoints.length - 1];
};

ms.face.prototype.getLooped = function() {
	return this.node.get('looped');
};

ms.face.prototype.getPolygons = function() {
	return this.node.get('bspPolygon');
};

ms.face.prototype.getFaceType = function() {
	return this.faceType;
};

ms.face.prototype.setLooped = function(looped) {
	this.node.setValue('looped', looped);
	this.node.stats.updateFace(this, !looped);
	this.dirty = true;
};

ms.face.prototype.getGroup = function() {
	return this.node.get('faceGroup');
};

ms.face.prototype.setHole = function(hole) {
	this.node.setValue('hole', hole);
};

ms.face.prototype.isHole = function() {
	return this.node.get('hole');
};

ms.face.prototype.isFullyMutable = function() {
	var endpoints = this.getEndpoints();
	return !endpoints.some(function(endpoint) {
		return !endpoint.getVertex().isMutable();
	});
};

ms.face.prototype.getPositions = function() {
	return this.getEndpoints().map((e) => { return e.getPosition(); });
};

ms.face.prototype.getNeighbors = function() {
	var neighbors = new Set();
	var endpoints = this.getEndpoints();
	endpoints.forEach(function(endpoint) {
		neighbors.add(endpoint.getTwin().getFace());
	});
	neighbors.delete(this);
	return Array.from(neighbors);
};

// Insert an endpoint into the face. It is added immediately after
// prevEndpoint which is assumed to be in the face. If prevEndpoint is null,
// it is added as the first endpoint.
ms.face.prototype.insert = function(endpoint, prevEndpoint) {
	var index = this.getEndpoints().indexOf(prevEndpoint);
	this.node.splice(endpoint, index + 1);
	this.dirty = true;
};

ms.face.prototype.append = function(faceB) {
	this.dirty = true;
	if (this == faceB) {
		this.setLooped(true);
		return;
	}
	
	var self = this;
	var endpointsB = faceB.getEndpoints().slice();	
	endpointsB.forEach(function(endpointB) {
		self.getNode().connect(endpointB);
	});
	faceB.getNode().destroy();
};

// Split the face at the endpoint. This endpoint becomes the first endpoint of the
// new face if unlooped, or the current face if looped.
ms.face.prototype.split = function(endpoint) {
	this.dirty = true;
	var endpoints = this.getEndpoints();
	var index = endpoints.indexOf(endpoint)
	if (this.getLooped()) {
		this.setLooped(false);
		var newOrder = endpoints.slice(index).concat(endpoints.slice(0, index));
		this.node.setArray(newOrder);
	} else {
		if (index == 0) {
			// ms.alert('Should not be splitting off all of the endpoints.');
			return;
		}
		var newFace = new ms.face(this.faceType, this.node.getStats());
		if (this.isHole()) {
			newFace.setHole(true);
		}
		// newFace.createGroup();
		var insertAtStart = !this.isHole();
		if (insertAtStart) {
			this.getGroup().getNode().splice(newFace, 0);
		} else {
			this.getGroup().getNode().connect(newFace);
		}
		var splitEndpoints = endpoints.slice(index);
		for (var i = 0; i < splitEndpoints.length; i++) {
			splitEndpoints[i].getNode().disconnect(this);
			splitEndpoints[i].getNode().connect(newFace);
		}
	}
};

// Due to dropping on the dimensions faceArea3D is not exactly equal to the area, but is proportional I think.
ms.face.prototype.signedArea = function() {
	if (ms.globalSettings.get('Use Network') && this.faceType) {
		return ms.shapeSet.faceArea3D(this.getPositions(), this.faceType.getMaxDim());
	} else {
		return ms.shapeSet.faceArea(this.getEndpoints());
	}
};

// Returns the face that this is enclosed in, if one exists.
// TODO: Cache the result so this is faster.
ms.face.prototype.enclosedFace = function() {	
	// eLeft is the leftmost bottom endpoint. pLeft is its position.
	var eLeft = this.leftmostEndpoint();
	var pLeft = eLeft.getPosition();	
	var rayCast = this.node.getModel().rayCastLeft(pLeft);
	if (!rayCast) {
		return null;
	}
	var leftFace = rayCast.endpoint.getFace()
	if (leftFace.signedArea() > 0) {
		return leftFace;
	} else {
		return leftFace.enclosedFace();
	}
};

ms.face.prototype.highlight = function(context, convertToScreen) {
	if (context instanceof ms.view3d) {
		return this.draw3D(context, convertToScreen, true);
	}
	if (!convertToScreen) {
		return;
	}
	for (var i = 0; i < this.getEndpoints().length; i++) {
		var endpoint = this.getEndpoints()[i];
		var line = endpoint.getLine();
		line && line.highlight(context, convertToScreen);
		endpoint.highlight(context, convertToScreen);
	}
};

// includeIndices means we are including the triangle indices rather than listing
// every position. This is for exporting to XML.
ms.face.prototype.fillRenderData = function(renderData, includeIndices) {
		if (this.getEndpoints().length == 0) {
		return;
	}

	var faceType = this.getEndpoints()[0].faceType();
	var area = null;
	if (this.dirty) {
		var endpoint = this.getEndpoints()[0];
		var isLeft = !endpoint.getIsAtStart();
		var edgeType = endpoint.getLine().getEdgeType();
		if (!edgeType.is3D()) {
			var area0 = isLeft ? edgeType.getLeftArea() : edgeType.getRightArea();
			if (area0 instanceof ms.area) {
				area = area0;
			}
		}
	}
	var dims = area ? 2 : 3;
	var renderPositions = [];
	var self = this;
	this.getEndpoints().forEach(function(endpoint) {
		endpoint.fillRenderPositions(renderPositions, dims);
	});

	var allData = null;
	var width = 1;
	var height = 1;
	if (area) {
		var areaTile = ms.decorationView.pickImage(area.get('Image'), this.imageSeed);
		if (areaTile && areaTile != 'None') {
			width = area.get('Width');
			height = area.get('Height');
			allData = renderData.getTextureData(areaTile);
		} else {
			var color = area.get('color');
			if (color && color != '#fff') {
				allData = renderData.getColorData(color);
			}
		}
	} else {
		allData = renderData.getTextureData('3D');
	}
	if (allData && faceType) {
		var maxDim = faceType.getMaxDim();
		ms.face.fillUsingEarcut(renderPositions, maxDim, allData, includeIndices, dims);
	}
};

ms.face.fillUsingEarcut = function(renderPositions, maxDim, allData, includeIndices, dims) {		
	var earcutPositions = [];
	for (var i = 0; i < renderPositions.length; i++) {
		if (i % 3 != maxDim) {
			earcutPositions.push(renderPositions[i]);
		}
	}
	
	var triangleIndices = earcut(earcutPositions, null, 2);
	if (includeIndices) {
		allData.positions = renderPositions;
		allData.triangleIndices = triangleIndices;
	} else {
		for (var i = 0; i < triangleIndices.length; i++) {
			var index = triangleIndices[i];
			if (dims == 2) {
				var x = renderPositions[2 * index];
				var y = renderPositions[2 * index + 1];
				allData.texcoords.push(x / width);
				allData.texcoords.push(y / height);
				allData.positions.push(x);
				allData.positions.push(y);
			} else {
				allData.positions.push(renderPositions[3 * index]);
				allData.positions.push(renderPositions[3 * index + 1]);
				allData.positions.push(renderPositions[3 * index + 2]);
			}
		}
	}
	
};

// Determine if faceB is fully inside this face.
ms.face.prototype.containsFace = function(faceB) {
	var positionsA = this.getPositions();
	var positionsB = faceB.getPositions();
	var maxDim = this.faceType.getMaxDim();
	if (ms.intersector.faceIntersect(positionsA, positionsB, maxDim)) {
		return false;
	}
	return ms.intersector.onFace(positionsA, positionsB[0], this.faceType.normal, this.faceType.u);
};

// Determine if faceB is fully outside this face.
ms.face.prototype.outsideFace = function(faceB) {
	var positionsA = this.getPositions();
	var positionsB = faceB.getPositions();
	var maxDim = this.faceType.getMaxDim();
	if (ms.intersector.faceIntersect(positionsA, positionsB, maxDim)) {
		return false;
	}
	// return true;
	// TODO: Determine if we need to check if one is not inside the other.
	return !ms.intersector.onFace(positionsA, positionsB[0], this.faceType.normal, this.faceType.u);
};

ms.face.prototype.draw3D = function(view, offset, highlighted) {
	if (this.getEndpoints().length == 0) {
		return;
	}
	var renderData = new ms.renderData();
	this.fillRenderData(renderData, false);
	var positions = renderData.data['3D'].positions;
	
	var color = [1, 0.5, 0.5, 1];
	if (this.getEndpoints().length == 0) {
		return;
	}
	var faceType = this.getEndpoints()[0].faceType();
	if (!faceType) {
		return;
	}
	var color = faceType.normalColor();
	if (highlighted) {
		color = [1, 0, 1, 1];
	}
	for (var i = 0; i < positions.length / 9; i++) {	
		view.drawTriangle(positions.slice(9 * i, 9 * (i + 1)), color);
	}
};


ms.face.prototype.draw = function(context, convertToScreen, highlighted) {
	if (context instanceof ms.view3d) {
		return this.draw3D(context, convertToScreen);
	}
	if (!this.getLooped() || this.signedArea() <= 0) {
		return;
	}
	var drawPoint = function(position) {
		var screenPosition = convertToScreen(position);
		context.lineTo(screenPosition.x, screenPosition.y);
	};

	context.beginPath();

	var p = convertToScreen(this.getEndpoints()[0].getVertex().getPosition());
	context.moveTo(p.x, p.y);
	var prevArea = null;
	var self = this;
	this.getEndpoints().forEach(function(endpoint) {
		endpoint.drawFace(drawPoint);

		var isLeft = !endpoint.getIsAtStart();
		var edgeType = endpoint.getLine().getEdgeType();
		var area = isLeft ? edgeType.getLeftArea() : edgeType.getRightArea();
		
		// context.fillStyle = ms.brushCollection.getColor(numFaceConflicts, true);
		if (area) {
			// TODO: This could be faster if we only computed the face color once.
			if (prevArea && prevArea != area) {
				ms.alert('Areas mismatch in the face.');
			}
			prevArea = area;
			var getPosition = function() {
				if (area.get('Fixed Origin')) {
					return ms.vec2.ORIGIN;
				} else {
					var position = self.leftmostEndpoint().getPosition();
					return convertToScreen(position);
				}
			};
			context.fillStyle = ms.exampleFace.getFillStyle(context, area, convertToScreen.scale, getPosition, self.imageSeed);
		} else {
			context.fillStyle = ms.globalSettings.get('Show Grid') ? "rgb(230, 230, 230)" : '#fff';
		}
	});
	context.closePath();
	context.fill();
};

ms.face.prototype.findClosibility = function() {
	var endpoints = this.getEndpoints();
	var endpoint0 = endpoints[0];
	var position0 = endpoint0.getPosition();
	var angle0 = endpoint0.getAngle();
	var edgeType0 = endpoint0.getEdgeType();
	var twinFace = endpoint0.getTwin().getFace();

	
	var indexWeight = ms.globalSettings.get('Closible Endpoint Index');
	var angleWeight = ms.globalSettings.get('Closible Angle');

	var lowestCost = 2 * angleWeight;
	var lowEndpoint;
	var done = 0; 
	var index = endpoints.length - 1;
	while (!done) {
		var endpointI = endpoints[index];
		if (endpointI.getEdgeType() == edgeType0 && endpointI.getTwin().getFace() == twinFace) {
			var positionI = endpointI.getPosition();
			var angleI = endpointI.getAngle();
			var angleP = ms.vec2.angle(positionI, position0);
			var deltaI = ms.util.angleDifference(angle0, angleI);
			var deltaP = ms.util.angleDifference(angle0, angleP);
			var costI = angleWeight * (1 - Math.cos(deltaI)) / 2;
			var costP = angleWeight * (1 - Math.cos(deltaP)) / 2;
			// var costI = angleWeight * Math.pow(1 - Math.cos(deltaI), 2) / 4;
			// var costP = angleWeight * Math.pow(1 - Math.cos(deltaP), 2) / 4;
			var cost = indexWeight * (index - endpoints.length + 1) + costI + costP;
			if (cost < lowestCost) {
				lowestCost = cost;
				lowEndpoint = endpointI;
			}
		}
		index --;
		if (!endpointI.getVertex().isMutable()) {
			done = true;
		}
	}
};

// Finds the leftmost bottom endpoint.
ms.face.prototype.leftmostEndpoint = function() {
	return ms.face.leftmostEndpoint(this.getEndpoints());
};

ms.face.leftmostEndpoint = function(endpoints) {
	// eLeft is the leftmost bottom endpoint. pLeft is its position.
	var eLeft = endpoints[0];
	var pLeft = eLeft.getPosition();
	for (var i = 1; i < endpoints.length; i++) {
		var pi = endpoints[i].getPosition();
		if (pi.x < pLeft.x || (pi.x == pLeft.x && pi.y < pLeft.y)) {
			eLeft = endpoints[i];
			pLeft = pi;
		}
	}
	return eLeft;
};

// Returns true if the connection is valid or there is no connection.
// Returns false if the connection is between two conflicting areas. 
ms.face.prototype.updateConnection = function() {
	var oldConnection = this.node.get('faceConnection');
	if (oldConnection) {
		oldConnection.getNode().destroy();
	}
	
	if (this.signedArea() >= 0) {
		return true;
	}
	// eLeft is the leftmost bottom endpoint. pLeft is its position.
	var eLeft = this.leftmostEndpoint();
	var pLeft = eLeft.getPosition();
	var rayCast = this.node.getModel().rayCastLeft(pLeft);
	var leftPosition = rayCast ? rayCast.position : new ms.vec2(0, pLeft.y);
	var connection = new ms.faceConnection(this.node.stats, [leftPosition, pLeft]);
	this.getNode().connect(connection);
	connection.addCells();

	var area0;
	if (rayCast) {
		var leftBound = rayCast.endpoint;
		leftBound.getNode().spliceInsert(0, leftBound.spliceIndex(connection), connection);
		var leftType  = leftBound.getEdgeType();
		area0 = leftBound.getIsAtStart()  ? leftType.getRightArea() : leftType.getLeftArea();
	} else {
		area0 = null;
	}
	var rightBound = eLeft;
	rightBound.getNode().spliceInsert(1, 0, connection);
	var rightType = rightBound.getEdgeType();
	var area1 = rightBound.getIsAtStart() ? rightType.getRightArea() : rightType.getLeftArea();
	if (area1 && area1.get('Boundary')) {
		area1 = null;
	}
	
	return area0 == area1;
};

// The bounding box of all the points with respect to a particular direction. 
ms.face.prototype.dirBounds = function(dir) {
	var low = Infinity;
	var high = -Infinity;
	var endpoints = this.getEndpoints();
	for (var i = 0; i < endpoints.length; i++) {
		var d = dir.dot(endpoints[i].getPosition());
		low = Math.min(d, low);
		high = Math.max(d, high);
	}
	return new ms.range(low, high);
};

// The bounding box of all the points with respect to a particular dimension. 
ms.face.prototype.dimBounds = function(dim) {
	var low = Infinity;
	var high = -Infinity;
	var endpoints = this.getEndpoints();
	for (var i = 0; i < endpoints.length; i++) {
		var d = endpoints[i].getPosition().getValue(dim);
		low = Math.min(d, low);
		high = Math.max(d, high);
	}
	return new ms.range(low, high);
};

// Pick a random point on the face. 
ms.face.prototype.randomPoint = function() {
	var maxDim = this.faceType.getMaxDim();
	var n = this.faceType.normal;
	var bounds = [];
	var sample = new ms.vec3(0, 0, 0);
	for (var i = 0; i < 3; i++) {
		if (i == maxDim) {
			sample.setValue(0, i);
		} else {
			sample.setValue(this.dimBounds(i).sample(), i);
		}
	}
	var p0 = this.getEndpoints()[0].getPosition();
	var s = (n.dot(p0) - n.dot(sample)) / n.getValue(maxDim);
	sample.setValue(s, maxDim);
	return sample;
};

ms.face.createFromPositions = function(positions, nodeStats, edgeType) {
	var faceType = edgeType.getFaceData()[1].type;
	var newFace = new ms.face(null, nodeStats);

	// So many hacks in one place. Just draw the damn face!
	var fakeVertexType = new ms.vertexType(new ms.vertexDecoration(() => {}));
	var diff0 = positions[0].copy().minus(positions[1]);
	var diff2 = positions[2].copy().minus(positions[1]);
	var maxDim = ms.util.maxDim(diff0.cross(diff2));
	// var fakeFaceType = { getMaxDim: () => { return maxDim;}};
	var fakeFaceData = [{type: faceType, onRight: false}, {type: faceType, onRight: true}];
	var fakeEdgeType = new ms.edgeType3D(fakeFaceData, null, {angle: 1});
	var fakeLine = { getEdgeType: () => edgeType }; // fakeEdgeType };
	
	for (var i = 0; i < positions.length; i++) {	
		var vertex = ms.vertex.createWithState(nodeStats, positions[i], 0, 1, fakeVertexType);
		
		var endpoint = new ms.endpoint(nodeStats, true, null, 0, ms.vec3.ORIGIN, 1, false, 0);
		endpoint.getNode().connect(vertex);
		newFace.getNode().connect(endpoint);
		endpoint.faceType = () => { return faceType;};
		endpoint.getLine = () => { return fakeLine; }
	}
	return newFace;
};

ms.face.prototype.print = function() {
	/* this.getEndpoints().forEach(function(endpoint) {
		endpoint.print();
	});
	window.console.log(this.getLooped() ? 'Looped' : 'Unlooped'); */
	ms.highlight(this);
};
