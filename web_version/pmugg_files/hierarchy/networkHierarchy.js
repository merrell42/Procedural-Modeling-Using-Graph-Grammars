ms.networkHierarchy = function() {
	this.reset();
};

ms.networkHierarchy.prototype.reset = function() {
	this.generations = [];
	this.nodeQueue = [];
	this.edgeOptions = {};
	this.gluingOptions = {};
	this.matcher = null;
	this.emptyNet = null;

	this.types = null;
	this.transitions = [];
	this.starterTransitions = [];
	this.groundTransitions = [];
	// TODO: Use networks to determine if grounded.
	this.grounded = ms.globalSettings.get('Grounded');

	this.starterNets = null;
};

ms.networkHierarchy.prototype.getTransition = function() {
	var transition = ms.pick(this.transitions);
	if (transition) {
		var networks = transition.getNetworks();
		var n = networks.length;
		// Pick two unique indices.
		var start = ms.random(n);
		var end = ms.random(n - 1);
		if (end >= start) {
			end++;
		}
		return {startNet: networks[start], endNet: networks[end]};
	}
	return null;
};

ms.networkHierarchy.prototype.getRemoveTransition = function() {
	var grounded = (this.starterTransitions.length == 0);
	if (grounded) {
		return null;
	}
	// var transitions = grounded ? this.groundTransitions : this.starterTransitions;
	var transitions = this.starterTransitions;
	var transition = ms.pick(transitions);
	if (transition) {
		var n = transition.getNetworks().length;
		var endNet = this.emptyNet;
		var startNet = transition.getNetworks()[ms.random(n - 1) + 1];
		return {startNet, endNet}
	}
	return null;
};

ms.networkHierarchy.prototype.getStarterTransition = function() {
	var firstTask = (ms.guideMutator.taskCount <= 1);
	var grounded = (this.grounded && firstTask) || (this.starterTransitions.length == 0);
	var transitions = grounded ? this.groundTransitions : this.starterTransitions;
	var transition = ms.pick(transitions);
	if (transition) {
		var n = transition.getNetworks().length;
		var startNet = transition.getNetworks()[0];
		var endNet = transition.getNetworks()[ms.random(n - 1) + 1];
		return {startNet, endNet, ground: transition.ground};
	}
	return null;
};

ms.networkHierarchy.partialImport = function(json, decoration) {
	var hierarchy = new ms.networkHierarchy();
	var types = { faceTypes: {}, edgeTypes: {}, vertexTypes: {}};
	json.faceTypes.forEach((faceJson) => {
		var faceType = ms.faceType3D.partialImport(faceJson);
		types.faceTypes[faceType.id] = faceType;
	});
	json.edgeTypes.forEach((edgeJson) => {
		var edgeType = ms.edgeType3D.partialImport(edgeJson, types.faceTypes);
		types.edgeTypes[edgeType.id] = edgeType;
	});
	var vertexTypes = json.vertexTypes.map((vertexJson) => ms.vertexType.partialImport(vertexJson, types.edgeTypes));
	hierarchy.types = { vertexTypes	};
	
	if (decoration) {
		var vDecorations = ms.exporter.makeArray(decoration.decoration.vDecoration);
		vertexTypes.forEach((vertexType) => {
			var id = vertexType.id;
			vDecorations.forEach((vDecoration) => {
				var types = vDecoration['@attributes'].types;
				if (ms.exporter.activeAction(types, id)) {
					vertexType.decoration.importXml(vDecoration);
				}
			});
		});
		var eDecorations = ms.exporter.makeArray(decoration.decoration.edgeBrush);
		Object.values(types.edgeTypes).forEach((edgeType) => {
			var id = edgeType.idNum;
			eDecorations.forEach((eDecoration) => {
				var types = eDecoration['@attributes'].types;
				if (ms.exporter.activeAction(types, id)) {
					edgeType.brush.importXml(eDecoration);
				}
			});
		});
	}
	
	var networks = {};
	networks.face = Object.values(types.faceTypes).map(ms.networkFactory.createFacePrimitive);
	networks.edge = Object.values(types.edgeTypes).map(ms.networkFactory.createEdgePrimitive);
	networks.vertex = vertexTypes.map(ms.networkFactory.createVertexPrimitive);
	networks.edgeTypes = Object.values(types.edgeTypes);	
	networks.splicedEdgeTypes = {};
	// Similar to ms.shape3d.
	networks.getSpliceEdgeType = function(faceType, dir) {
		return ms.shape3D.getSpliceEdgeType(networks.splicedEdgeTypes, networks.edge, faceType, dir);
	};
	
	var vertexNetworks = networks.vertex;
	
	// var generation1 = vertexNetworks.map((network) => (new ms.hierarchyNode(network)));
	// hierarchy.generations = [generation1];

	var connectionOrder = [];
	for (var i = 0; i < vertexTypes.length; i++) {
		var vertexType = vertexTypes[i];
		var connectors = vertexNetworks[i].getConnectors();
		var order = [];
		vertexType.connections.forEach((connection) => {
			var index = connectors.findIndex((connector) => (connector.connection == connection));
			if (index == -1) {
				ms.alert('Can not find a connection.');
			}
			order.push(index);
		});
		function areAllUnique(arr) {
			// Use a Set to store unique values
			let uniqueSet = new Set(arr);
			return arr.length === uniqueSet.size;
		}
		if (!areAllUnique(order)) {
			ms.alert('Duplicate connections');
		}
		connectionOrder.push(order);
	}

	var transitionNets = [];
	json.matches.map((match) => {
		var glued = ms.networkHierarchy.glueMatch(match, vertexNetworks, connectionOrder);
		if (glued) {
			transitionNets.push(glued);
		}
	});
	hierarchy.partialGenerate(transitionNets, networks);
	// Filter out any transitions that create floating objects.
	hierarchy.starterTransitions = hierarchy.starterTransitions.filter((t) => t.networks[0].interior.faces.length > 0);
	
	var faceTypes = Object.values(types.faceTypes);
	// Sort the face types. The ground should be before the other flat types.
	// TODO: Use the name instead.
	faceTypes.sort((a, b) => parseInt(a.id.split(',')[0] - parseInt(b.id.split(',')[0])));

	var groundType = Object.values(faceTypes).find((faceType) => {
		return faceType.normal.z > 0.999;
	});
	if (!groundType) {
		ms.alert('No ground type found.');
	} else {
		hierarchy.groundTransitions = [ms.testRunner.getGroundTransition(groundType)];
	}
	
	return hierarchy;
};

ms.networkHierarchy.glueMatch = function(match, vertexNetworks, connectionOrder) {
	var matchVertices = match.vertices.map((index) => (vertexNetworks[index].copy()));
	var matchToNetwork = {};
	var networkToMatch = {};
	var networkMap = {};
	for (var i = 0; i < matchVertices.length; i++) {
		var connectorsI = matchVertices[i].getConnectors();
		for (var j = 0; j < connectorsI.length; j++) {
			var matchKey = match.vertices[i] + ',' + j;
			var netKey = matchVertices[i].id + ',' + j;
			matchToNetwork[matchKey] = netKey;
			networkToMatch[netKey] = matchKey;
		}
		networkMap[matchVertices[i].id] = matchVertices[i];
	}
	var edgeQueue = match.edges.slice();
	
	var finalResult = null;
	while (edgeQueue.length > 0) {
		var nextQueue = [];
		edgeQueue.forEach((edge) => {
			connectionOrder[match.vertices[5]]
			var vertexA = match.vertices[edge[0]];
			var connectA = connectionOrder[vertexA][edge[1]];
			var vertexB = match.vertices[edge[2]];
			var connectB = connectionOrder[vertexB][edge[3]];
			var [netA, nConA] = matchToNetwork[vertexA + ',' + connectA].split(',');
			var [netB, nConB] = matchToNetwork[vertexB + ',' + connectB].split(',');
			var networkA = networkMap[netA];
			var networkB = networkMap[netB];
			var connectorA = networkA.getConnectors()[nConA];
			var connectorB = networkB.getConnectors()[nConB];
			
			if (netA == netB) {
				var loopables = networkA.findLoopables();
				var canLoop = loopables.some((loopable) => {
					return (loopable[0] == connectorA && loopable[1] == connectorB) ||
						   (loopable[1] == connectorA && loopable[0] == connectorB);
				});
				// Skip this edge and add it to the queue, if it cannot be looped.
				if (!canLoop) {
					nextQueue.push(edge);
					return;
				}
			}
			var result = connectorA.copyAndGlue(connectorB, netA == netB);
			var newNetId = result.network.id;
			networkMap[newNetId] = result.network;
			result.track.aDest.forEach((a, index) => {
				if (a >= 0) {
					var matchKey = networkToMatch[netA + ',' + index];
					var newNetKey = result.network.id + ',' + a;
					matchToNetwork[matchKey] = newNetKey;
					networkToMatch[newNetKey] = matchKey;
				}
			});
			if (netA != netB) {
				result.track.bDest.forEach((b, index) => {
					if (b >= 0) {
						var matchKey = networkToMatch[netB + ',' + index];
						var newNetKey = result.network.id + ',' + b;
						matchToNetwork[matchKey] = newNetKey;
						networkToMatch[newNetKey] = matchKey;
					}
				});
			}
			finalResult = result.network;
		});
		// If no progress has been made, this cannot be glued together.
		if (edgeQueue.length == nextQueue.length) {
			return null;
		}
		edgeQueue = nextQueue;
	}
	
	return finalResult;
};

ms.networkHierarchy.import = function(json) {
	var hierarchy = new ms.networkHierarchy();
	hierarchy.types = ms.shape3D.import(json.types);
	var imp = (transition) => (ms.netTransition.import(transition, hierarchy.types));
	hierarchy.transitions = json.transitions.map(imp);
	hierarchy.starterTransitions = json.starterTransitions.map(imp);
	hierarchy.groundTransitions = json.groundTransitions.map(imp);
	hierarchy.grounded = json.grounded;
	hierarchy.emptyNet = ms.boundNet.import(json.emptyNet, hierarchy.types);
	return hierarchy;
};

ms.networkHierarchy.prototype.draw = function(view, offset) {
	var transitions = this.groundTransitions.concat(this.starterTransitions, this.transitions);
	var margin = 50;
	var highlightedSize = ms.graph.HIGHLIGHTED_SIZE;
	offset = new ms.vec2(offset.x + margin, offset.y + margin + ms.graph.HIGHLIGHTED_SIZE);
	var w = ms.mainController.getCanvasWidth() - margin;
	var h = ms.mainController.getCanvasHeight() - margin;

	var generations = this.generations.map((generation) => {
		if (ms.globalSettings.get('Show Inactives')) {
			return generation;
		} else {
			return generation.filter((node) => { return node.getActive(); });
		}
	});
	var allNodes = [];
	generations.forEach((generation) => {
		allNodes = allNodes.concat(generation);
	});
	
	var numCols = ms.familyTree.lowGenerationColumns;
	var populationSize = allNodes.length;
	var numCols = Math.max(ms.familyTree.lowGenerationColumns, Math.floor(Math.sqrt(populationSize)), transitions.length); 
	var generationRows = Math.ceil(populationSize / numCols);
	var numRows = 1 + ms.familyTree.ringTransitionRows + generationRows;

	var displaySize = 85;
	offset = new ms.vec2(10, 10);
	var extraSpacing = 5;

	var options = {};
	
	console.log('# of network transitions: ' + transitions.length);

	var transitionCols = 12;
	// Draw the transitions.
	var col = 0;
	var row = 1;
	for (var i = 0; i < transitions.length; i++) {
		var networks = transitions[i].getNetworks();
		var r = row;
		for (var j = 0; j < networks.length; j++) {
			var network = networks[j];
			options.rect = [
				 col      * displaySize + offset.x,
				(col + 1) * displaySize + offset.x,
				h - ((r + 1) * displaySize + offset.y),
				h - ( r      * displaySize + offset.y),
				0, displaySize];
			options.lineWidth = 1;
			network.draw(view, options);
			
			if (j == 0) {
				var arrowLength = 0.15;
				var start = new ms.vec3(
					(col + 0.5) * displaySize + offset.x,
					h - ((r + 1.5 - arrowLength) * displaySize + offset.y), 
					displaySize / 2);
				var middle = new ms.vec3(
					(col + 0.5) * displaySize + offset.x,
					h - ((r + 1.5) * displaySize + offset.y), 
					displaySize / 2);
				var end = new ms.vec3(
					(col + 0.5) * displaySize + offset.x,
					h - ((r + 1.5 + arrowLength) * displaySize + offset.y), 
					displaySize / 2);
				// This is happening inside of network.draw.
				view.swapY(start);
				view.swapY(middle);
				view.swapY(end);
				if (view.drawLine) {
					view.drawLine('#000', start, end, ms.vec2.ORIGIN, {hasArrows: [true, true], arrowSize: 10});	
				} else {				
					// For view3D.
					view.drawArrow(middle, start, '#000');
					view.drawArrow(middle, end, '#000');	
				}
			}
			// if (graph.getLink('is3D')) { r--; } else {
			r += 2;
		}
		col++;
		if (col % transitionCols == 0 && i != transitions.length - 1) {
			col = 0;
			row += 4;
		}
	}
	if (!ms.mvp) {
		this.drawGenerations(view, offset, generations, row, displaySize, numCols, options, h);
	}
};