ms.familyTree = function() {
	this.reset();
};

ms.familyTree.prototype.reset = function() {
	this.allNodes = [];
	this.leafNodes = [];
	this.edgeNodes = null;
	this.edgeTypes = null;
	this.allEdgeTypes = null;
	this.vertexTypes = null;
	this.vertexGraphs = null;
	
	this.ringTable = [];
	this.ringTransitions = [];
	this.backwardMatches = [];
	this.stubs = []; // Rings with one endpoint.
	this.mutableRings = []; // Rings that are part of some transition.
	this.treeRings = []; // Rings that are part of the ringTree.
	this.emptyRing = new ms.ring([], [], [], new ms.graph());
	this.edgeToEndpointsMap = [];
	this.ringTree = null;
	this.boundaryGroups = null;
};

ms.familyTree.prototype.export = function() {
	var faceTypes = [];
	this.allEdgeTypes.forEach((edgeType) => {
		var areas = [edgeType.getLeftArea(), edgeType.getRightArea()];
		areas = areas.filter((area) => { return area && area.export; });
		ms.union(faceTypes, areas);
	});
	var rings = this.treeRings;
	rings.slice().forEach((ring) => {
		ring.findRings(rings);
	});
	this.boundaryGroups.forEach((bGroup) => {
		bGroup.findRings(rings);
	});	
	
	var types = {vertexTypes: this.vertexTypes, edgeTypes: this.allEdgeTypes, faceTypes, rings};
	var ringTree = {};
	Object.keys(this.ringTree).forEach((key) => {
		ringTree[key] = this.ringTree[key].export(rings, this.vertexTypes);
	});
	var result = {
		useNetworks: false,
		faceTypes: types.faceTypes.map((type) => type.export()),
		edgeTypes: types.edgeTypes.map((type) => type.export(types)),
		vertexTypes: types.vertexTypes.map((type) => type.export(types)),
		ringTree,
		rings: this.treeRings.map((ring) => ring.export(types)),
		emptyRing: this.treeRings.indexOf(this.emptyRing),
		boundaryGroups: this.boundaryGroups.map((g) => g.export(types)),
	}
	return result;
};

ms.familyTree.import = function(json) {
	var types = {};
	types.faceTypes = json.faceTypes.map((type) => ms.area.import(5, [type], () => {}));
	types.edgeTypes = json.edgeTypes.map((type) => ms.edgeType.import(type, types));
	types.vertexTypes = json.vertexTypes.map((type) => ms.vertexType.import(type, types));
	types.rings = json.rings.map((ring) => ms.ring.import(ring, types));
	types.rings.forEach((ring, index) => ring.importSpots(json.rings[index].transitionSpots, types));

	var result = new ms.familyTree();
	result.ringTree = {};
	Object.keys(json.ringTree).forEach((key) => {
		result.ringTree[key] = ms.ringTreeNode.import(json.ringTree[key], types.rings, types);
	});

	result.emptyRing = types.rings[json.emptyRing];
	result.boundaryGroups = json.boundaryGroups.map((group) => ms.ringGroup.import(group, types));
	return result;
};

ms.familyTree.ringTransitionRows = 4;
ms.familyTree.lowGenerationColumns = 10;
ms.familyTree.highGenerationColumns = 25;
ms.familyTree.windingNumberPrecision = 1e-4;
ms.familyTree.MARGIN = 0.05; // As a fraction of the box size.

ms.familyTree.prototype.generate = function(graphs) {
	var edgeTypes = graphs.edgeTypes;
	this.allNodes = [];
	this.leafNodes = [];
	this.stubs = [];
	this.ignoredIfWindingDisabled = false;
	this.edgeTypes = edgeTypes;
	this.vertexTypes = graphs.vertexTypes;
	this.allEdgeTypes = graphs.allEdgeTypes;
	for (var i = 0; i < edgeTypes.length; i++) {
		var stub = [null, null];
		var brush = edgeTypes[i].getBrush();
		var loopy = brush ? brush.get('Loopy') : true;
		stub.loopy = loopy;
		this.stubs[edgeTypes[i].id] = stub;
		
	}
	this.createInitialGenerations(graphs);
	var numGenerations = 2;
	var scoreCap = 2;
	while (numGenerations <= ms.globalSettings.get('Generations')) {
		var hasNewNodes = this.createNextGeneration(scoreCap);
		// Infinity is sometimes appropriate for some of the streams. Hopefully this will be fixed
		// with a non-loopy brush.
		// var hasNewNodes = this.createNextGeneration(Infinity);
		if (!hasNewNodes) {
			scoreCap++;
		}
		var filledStubs = this.filledStubs();
		if (filledStubs) {
			this.reset();
			this.generateUsingStubs(filledStubs, graphs, edgeTypes);
			this.createRingTree();
			return;
		}
		// document.getElementById('statistics').innerHTML = numGenerations;
		numGenerations++;
	}


	var starterTransitions = this.ringTransitions.filter(function(transition) {
		return transition.getGroups()[0].getRings()[0].getGraph().isEmpty();
	});
	
	if (starterTransitions.length == 0) {
		ms.alert('No starter transitions');
	} else if (starterTransitions.length > 1) {
		ms.alert('Too many transitions');
	}
	
	// Remove the boundary groups.
	var starterTransition = starterTransitions[0];
	var boundaryGroups = [];
	
	if (starterTransition) {
		starterTransition.groups.forEach(function(group) {
			var isBoundary = group.getGraph().getEdges().some(function(edge) {
				var brush = edge.getCore().getBrush();
				return brush && brush.get('Boundary');
			});
			if (isBoundary) {
				boundaryGroups.push(group);
			}
		});
		starterTransition.removeGroups(boundaryGroups);
	}
	this.boundaryGroups = boundaryGroups;
	
	this.createRingTree();
};

ms.familyTree.prototype.createRingTree = function() {
	var self = this;
	var changeableRings = [];
	var destructableRings = [];
	this.mutableRings.forEach((ring) => {
		var changeable = false;
		var destructable = false;
		ring.getTransitionSpots().forEach(function(spot) {
			if (spot.transition.isDestructible()) {
				destructable = true;
			} else {
				changeable = true;
			}
		});
		if (destructable) {
			var copy = ring.copy();
			destructableRings.push(copy);
			copy.transitionSpots = ring.transitionSpots.filter(function(spot) {
				return spot.transition.isDestructible();
			});
		}
		if (changeable) {
			changeableRings.push(ring);
			ring.transitionSpots = ring.transitionSpots.filter(function(spot) {
				return !spot.transition.isDestructible();
			});
		}
	});
	
	this.ringTree = {};
	this.ringTree['change'] = ms.ringTreeNode.createTree(changeableRings);
	this.ringTree['destroy'] = ms.ringTreeNode.createTree(destructableRings);
	var self = this;
	changeableRings.forEach(function(ring) {
		var multipleTransition = ring.getTransitionSpots().some(function(spot) {
			return spot.transition.groups[spot.groupIndex].rings.length > 1;
		});
		if (multipleTransition) {
			self.ringTree[ring.id] = ms.ringTreeNode.createTree([ring]);
		}
	});
	this.treeRings = [this.emptyRing].concat(changeableRings).concat(destructableRings);
};

ms.familyTree.prototype.filledStubs = function() {
	if (!ms.globalSettings.get('Stubs Enabled')) {
		return null;
	}

	var self = this;
	var isFilled = false;
	var filledStubs = {};
	this.stubs.forEach(function(stub, index) {
		if (stub.loopy ? stub[0] && stub[1] : stub[0] || stub[1]) {
			isFilled = true;
			filledStubs[index] = stub;
		}
	});
	if (isFilled) {
		return filledStubs;
	} else {
		return null;
	}
};

ms.familyTree.prototype.addRingTransition = function(transition) {
	var groups = transition.getGroups();
	var self = this;
	groups.forEach(function(group) {
		self.addMutableRings(group);
	});
	this.ringTransitions.push(transition);
};

ms.familyTree.prototype.generateUsingStubs = function(filledStubs, graphs, edgeTypes) {
	var vertexGraphs = [];
	graphs.vertex.forEach(function(graph) {
		vertexGraphs = vertexGraphs.concat(ms.familyTree.attachStubs(graph, filledStubs));
	});
	this.generate({vertex: vertexGraphs, edge: graphs.edge}, edgeTypes);

	var keys = Object.keys(filledStubs);
	for (var i = 0; i < keys.length; i++) {
		var id = keys[i];
		var edgeType = edgeTypes.find(function(type) { return type.id == id; });
		var stub = filledStubs[id];
		if (stub.loopy) {
			var lineGraph = ms.graph.createSingleEdgeGraph(edgeType);
			var lineRing = ms.ring.create(lineGraph);
			var lineGroup = ms.ringGroup.createFromRing(lineRing);
			
			var ring0 = ms.ring.create(stub[0]);
			var ring1 = ms.ring.create(stub[1]);
			var stubGroup = ms.ringGroup.createFromRings([ring0, ring1]);
			var transition = new ms.ringTransition([lineGroup, stubGroup]);
			this.addRingTransition(transition);
		}
	}
};

ms.familyTree.attachStubs = function(graph, filledStubs) {
	var outerEndpoints = graph.getOuterEndpoints();
	
	var stubTable = {};
	for (var i = 0; i < outerEndpoints.length; i++) {
		var endpoint = outerEndpoints[i];
		var edgeType = endpoint.getEdge().getCore();
		var stub = filledStubs[edgeType.id];
		
		var edgeIndex = endpoint.edgeIndex;
		var directedStub = stub && stub[1 - edgeIndex];
		if (directedStub) {
			var stubElement = stubTable[edgeType.id + ',' + edgeIndex];
			if (!stubElement) {
				// If this is loopy, apply the stub immediately. Otherwise we have to count the number of stubs.
				if (stub.loopy) {
					var attached = ms.graphEndpoint.attachEndpoints(endpoint, directedStub.getOuterEndpoints()[0]);
					return ms.familyTree.attachStubs(attached.newGraph, filledStubs);
				} else {
					// The non-loopy brushes are implemented. The whole reason for them was to attach the stubs
					// differently, but I'm having trouble getting it to work.
					ms.alert('Stubs only implemented for loopy brushes.');
				}
				stubElement = {
					stub: directedStub,
					endpoints: [],
				};
				stubTable[edgeType.id] = stubElement
			}
			stubElement.endpoints.push(endpoint);
		}
	}

	/* var stubValues = Object.values(stubTable);
	stubValues = stubValues.filter(function(sValue) {
		return sValue.endpoints.length >= 2;
	});

	// Only process the first one. Slightly inefficient.
	if (stubValues.length > 0) {
		var stubValue = stubValues[0];
		var endpoints = stubValue.endpoints;
		var stub = stubValue.stub;
		var results = [];
		for (var i = 0; i < endpoints.length; i++) {
			for (var j = 0; j < endpoints.length; j++) {
				if (i != j) {
					ms.graphEndpoint.attachEndpoints(endpoints[j], stub.getOuterEndpoints()[0]);
				}
			}
		}
	} */
	
	/* for (var i = 0; i < outerEndpoints.length; i++) {
		var endpoint = outerEndpoints[i];
		var edgeType = endpoint.getEdge().getCore();		
		var brush = edgeType.getBrush();
		var loopy = brush ? brush.getLoopy() : true;
		var stub = filledStubs[edgeType.id];
		if (stub) {
			var edgeIndex = endpoint.edgeIndex;
			var attached = ms.graphEndpoint.attachEndpoints(endpoint, stub[1 - edgeIndex].getOuterEndpoints()[0]);
			return ms.familyTree.attachStubs(attached.newGraph, filledStubs);
		}
	} */
	return [graph];
};

ms.familyTree.typesToGraphs = function(vertexTypes, edgeTypes, mergeGround) {
	console.log('# Input Primitives: ' + vertexTypes.length);
	var newEdgeTypes = edgeTypes.slice();
	var vertexGraphs = vertexTypes.map(ms.graph.createSingleVertexGraph);
	ms.familyTree.applySuperVertex(newEdgeTypes, vertexGraphs, mergeGround);
	var edgeGraphs = newEdgeTypes.map(ms.graph.createSingleEdgeGraph);
	console.log('# Input Primitives (after super): ' + vertexGraphs.length);
	// Not sure if we need both allEdgeTypes and edgeTypes. edgeTypes removes some in the super vertex.
	return {vertex: vertexGraphs, edge: edgeGraphs, edgeTypes: newEdgeTypes, allEdgeTypes: edgeTypes, vertexTypes};
};

ms.familyTree.prototype.getEmptyRing = function() {
	return this.emptyRing;
};

ms.familyTree.prototype.getRingTransitions = function() {
	return this.ringTransitions;
};

ms.familyTree.prototype.getRingTree = function() {
	return this.ringTree;
};

ms.familyTree.prototype.removeLeaf = function(node) {
	ms.remove(node, this.leafNodes);
};

ms.familyTree.prototype.addNode = function(node, opt_isLeaf) {
	this.allNodes.push(node);
	if (opt_isLeaf !== false) {
		this.leafNodes.push(node);
	}
	this.addRings([node]);
}

ms.familyTree.prototype.addRing = function(ring) {
	var numAngles = ring.getAngles().length;
	while (this.ringTable.length <= numAngles) {
		this.ringTable[this.ringTable.length] = {};
	}
	var endpointIds = ring.getEndpointIds();
	var id = endpointIds.length > 0 ? endpointIds[0] : -1;
	var group = this.ringTable[numAngles];
	if (group.hasOwnProperty(id)) {
		group[id].push(ring);
	} else {
		group[id] = [ring];
	}
};

// Shift the endpoint data so that index is the first endpoint.
// This also shifts the winding angles.
ms.familyTree.shiftEndpointData = function(endpointData, index) {
	for (var i = 0; i < index; i++) {
		endpointData[i].winding += 2 * Math.PI;
	}
	var ending = endpointData.splice(index);
	endpointData = ending.concat(endpointData);
	var winding0 = endpointData[0].winding;
	for (var i = 0; i < endpointData.length; i++) {
		endpointData[i].winding -= winding0;
	}
	return endpointData;
};

ms.familyTree.ringEndpointData = function(ringA, indexA) {
	var endpointData = [];
	var idsA = ringA.getEndpointIds();
	var windingAnglesA = ringA.getWindingAngles();
	for (var i = 0; i < idsA.length; i++) {
		endpointData[i] = {index: i, id: idsA[i], winding: windingAnglesA[i]};
	}
	return ms.familyTree.shiftEndpointData (endpointData, indexA);
};

ms.familyTree.ringGroupEndpointData = function(groupB, indexB) {
	// The mapping is just to create a copy.
	var endpointData = groupB.getEndpointData().map(function(data, index) {
		return {index: index, id: data.id, winding: data.winding};
	});
	return ms.familyTree.shiftEndpointData (endpointData, indexB);
};

ms.familyTree.attachRingEndpointsToGroup = function(ringA, groupB, endpointDataA, endpointDataB) {
	var removeB = function(dataB) {
		groupB.insertEndpoint(null, dataB[0].index, -1);
		dataB.shift();
	};
	
	// If all ringA's endpoints are added, then we have success.
	if (endpointDataA.length == 0) {
		while (endpointDataB.length > 0) {
			removeB(endpointDataB);
		}
		return [groupB];
	}
	// If it's impossible to add all ringA's endpoints, then we have failed.
	if (endpointDataA.length > endpointDataB.length ) {
		return [];
	}
	var datumA = endpointDataA[0];
	var datumB = endpointDataB[0];
	var precision = ms.familyTree.windingNumberPrecision;
	if (datumA.id == datumB.id &&
		Math.abs(datumA.winding - datumB.winding) < precision) {
		// Consider two options: (1) accept this ID now or (2) see if we can find it later.
		var groupB2 = groupB.copy();
		endpointDataA2 = endpointDataA.slice();
		endpointDataB2 = endpointDataB.slice();

		groupB.insertEndpoint(ringA.getOuterEndpoints()[datumA.index], datumB.index, datumA.index);
		endpointDataA.shift();
		endpointDataB.shift();

		groupB2.insertEndpoint(null, datumB.index, -1);
		endpointDataB2.shift();

		var result1 = ms.familyTree.attachRingEndpointsToGroup(ringA, groupB,  endpointDataA,  endpointDataB);
		var result2 = ms.familyTree.attachRingEndpointsToGroup(ringA, groupB2, endpointDataA2, endpointDataB2);
		return result1.concat(result2);
	}
	// We cannot accept this ID. See if we can find it later.
	removeB(endpointDataB);
	return ms.familyTree.attachRingEndpointsToGroup(ringA, groupB, endpointDataA, endpointDataB);
};

ms.familyTree.prototype.fillGroupHoles = function(ring, group, edgeTypes) {
	var holeData = group.getHoleData();
	if (holeData.ids.length == 0) {
		// The number of single fragment counts must match unless this is a starter rule.
		if (ring.singleFragmentCount() == group.singleFragmentCount() ||
			ring.isEmpty() || group.isEmpty()) {
			return group;
		} else {
			return null;
		}
	}
	var idToSplittable = function(endpointId) {
		var id = Math.floor(endpointId / 2);
		var edgeType = edgeTypes.find(function(type) { return type.id == id; });
		return edgeType.splittable();
	};
	for (var i = 0; i < holeData.ids.length; i++) {
		// Do not allow multiple fragments for edges that require single fragments. Return null if the edgeType is a single fragment
		// and an existing endpoint is not a single fragment.
		if (!idToSplittable(holeData.ids[i])) {
			var existingNotSplittable = function(datum) {
				return datum.endpoint && !idToSplittable(datum.id);
			};
			if (group.getEndpointData().some(existingNotSplittable)) {
				return null;
			};
		}
	}
	
	var holeFiller = this.findRingMatch(ring, holeData.ids, holeData.windings);
	if (holeFiller) {
		group.fillHole(holeFiller);
		return this.fillGroupHoles(ring, group, edgeTypes);
	} else {
		return null;
	}
};

ms.familyTree.prototype.findRingMatch = function(ring, endpointIds, windingAngles) {
	if (endpointIds.length == 0) {
		var group = new ms.ringGroup([], []);
		group.addRing(this.ringTable[0][-1][0], true);
		return group;
	}
	// Find the indices of all ids matching the first one.
	var findIndices0 = function(ids) {
		var indices0 = [];
		ids.forEach(function(id, index) {
			if (id == ids[0]) {
				indices0.push(index);
			}
		});
		return indices0;
	};

	var smallestCost = Infinity;
	var smallestFilledGroup = null;
	for (var numEndpoints = endpointIds.length; numEndpoints > 0; numEndpoints--) {
		var startId = endpointIds[0];
		// Search through all the rings with a particular number of endpoints. 
		var tableEntry = this.ringTable[numEndpoints];
		var tableGroup = tableEntry && tableEntry[startId];
		if (tableGroup) {
			for (var i = 0; i < tableGroup.length; i++) {
				var ringA = tableGroup[i];
				var startIndices = findIndices0(ringA.getEndpointIds());
				for (var k = 0; k < startIndices.length; k++) {
					var indexA = startIndices[k];
					var groupB = new ms.ringGroup(endpointIds, windingAngles);
					var endpointDataA = ms.familyTree.ringEndpointData(ringA, indexA);
					var endpointDataB = ms.familyTree.ringGroupEndpointData(groupB, 0);
					var ringGroups = ms.familyTree.attachRingEndpointsToGroup(ringA, groupB, endpointDataA, endpointDataB);

					for (var j = 0; j < ringGroups.length; j++) {
						var group = ringGroups[j];
						group.addRing(ringA);
						var filledGroup = this.fillGroupHoles(ring, group, this.edgeTypes);
						if (filledGroup) {
							var cost = filledGroup.getCost();
							if (cost < smallestCost) {
								smallestCost = cost;
								smallestFilledGroup = filledGroup;
							}
							// There cannot be less than 1 rings. This is the smallest filled group.
							/* if (numRings == 1) {
								return filledGroup;
							} */
						}
					}
				}
			}
		}
	}
	return smallestFilledGroup;
};

ms.familyTree.prototype.applyBackwardsRemove = function(ringB) {
	for (var i = 0; i < this.ringTable.length; i++) {
		var entriesI = Object.entries(this.ringTable[i]);
		var filteredI = {};
		for (var j = 0; j < entriesI.length; j++) {
			var entryJ = entriesI[j];
			var filteredRings = entryJ[1].filter(function(ringA) {
				return !ringA.isDescendant(ringB) && ringA != ringB;
			});
			filteredI[entryJ[0]] = filteredRings;
		}
		this.ringTable[i] = filteredI;
	}

	// Not sure if we need to remove these transitions.
	for (var i = 0; i < this.ringTransitions.length; i++) {
		var transition = this.ringTransitions[i];
		var validGroups = [];
		var invalidGroups = [];
		transition.groups.forEach(function(group) {			
			var isValid = !group.getRings().some(function(ring) {
				return ring.isDescendant(ringB);
			});
			if (isValid) {
				validGroups.push(group);
			} else {
				invalidGroups.push(group);
			}
		});		
		if (validGroups.length == 1) {
			invalidGroups.push(validGroups[0]);
			validGroups = [];
		}
		transition.groups = validGroups;
		invalidGroups.forEach(function(group) {
			group.removeTransition(transition);
		});
	}
	this.ringTransitions = this.ringTransitions.filter(function(transition) {
		return transition.groups.length > 1;
	});
	this.mutableRings = this.mutableRings.filter(function(ring) {
		if (ring.isEmpty()) {
			return false;
		}
		if (ring.getTransitionSpots().length == 0) {
			return false;
		}
		return true;
	});
	
	if (ringB.node.parentEdges[0].parent.parentEdges.length == 0) {
		var removeGraph = ringB.getNode().parentEdges[0].attachment.getGraph();
		ms.remove(removeGraph, this.vertexGraphs);
	}
};

ms.familyTree.prototype.applyBackwardsAdd = function(allMatches) {
	var rings = [];
	allMatches.forEach(function(match) {
		match.group.getRings().forEach(function(ring) {
			if (!rings.includes(ring)) {
				rings.push(ring);
			}
		});
	});
	var newVertexGraphs = [];
	for (var i = 0; i < rings.length; i++) {
		var isDescendant = allMatches.some(function(matchB) {
			return rings[i].isDescendant(matchB.ring);
		});
		if (isDescendant) {
			newVertexGraphs.push(rings[i].getGraph());
		}
	}

	this.vertexGraphs = this.vertexGraphs.concat(newVertexGraphs);
	this.edgeToEndpointsMap = ms.familyTree.edgeToEndpointsMap(this.edgeTypes, this.vertexGraphs);	
	for (var i = 0; i < this.allNodes.length; i++) {
		var node = this.allNodes[i];
		// Recreate nodes that were destroyed. This is needed for intersecting boxes.
		// We might need to check that none of the recreated nodes are the nodes that were just
		// removed from backwards matching or their descendants.
		var recreate = this.edgeNodes.includes(node);
		node.createChildEdges(this.edgeToEndpointsMap, recreate);
	}
	var self = this;
	newVertexGraphs.forEach(function(vertexGraph) {
		self.addPrimaryVertexGraph(vertexGraph);
	});
};

// Remove any matches that are a descendant of another match.
ms.familyTree.filterBackwardMatches = function(matches) {
	if (matches.length == 0) {
		return [];
	}
	var filtered = [matches.shift()];
	while (matches.length > 0) {
		var matchA = matches.shift();
		var equals = filtered.some(function(matchB) {
			return matchA.ring == matchB.ring;
		});
		// Keep the earlier one if they're equal.
		if (equals) {
			continue;
		}
		// Remove A if it is a descendant.
		var aDescends = filtered.some(function(matchB) {
			return matchA.ring.isDescendant(matchB.ring);
		});
		if (aDescends) {
			continue;
		}
		// Remove B if it is a descendant.
		filtered = filtered.filter(function(matchB) {
			return !matchB.ring.isDescendant(matchA.ring);
		});
		filtered.push(matchA);
	}
	return filtered;
};

ms.familyTree.prototype.findBackwardMatch = function(ringA) {
	var matches = [];
	var endpointDataA = ms.familyTree.ringEndpointData(ringA, 0);
	for (var i = 0; i < this.allNodes.length; i++) {
		var nodeB = this.allNodes[i];
		var ringB = nodeB.ring;
		if (ringB && !nodeB.destroyed) {
			var endpointIdsA = ringA.getEndpointIds();
			var endpointIdsB = ringB.getEndpointIds();
			var windingAnglesB = ringB.getWindingAngles();
			// RingA must be pair with another with another ring. We'll call it ringC.
			// We assume that ringA and ringC must at least two endpoints.
			// They have no endpoint they're useless.
			// If they have only one, Stubs Enabled takes care of this.
			if (endpointIdsA.length <= endpointIdsB.length - 2 && endpointIdsA.length >= 2) {
				for (var indexB = 0; indexB < endpointIdsB.length; indexB++) {
					var groupB = new ms.ringGroup(endpointIdsB, windingAnglesB);
					var endpointDataB = ms.familyTree.ringGroupEndpointData(groupB, indexB);
					var ringGroups = ms.familyTree.attachRingEndpointsToGroup(ringA, groupB, endpointDataA.slice(), endpointDataB);

					for (var j = 0; j < ringGroups.length; j++) {
						var group = ringGroups[j];
						group.addRing(ringA);
						var filledGroup = this.fillGroupHoles(ringB, group, this.edgeTypes);
						if (filledGroup) {
							matches.push({ring: ringB, group: filledGroup});
						}
					}
				}
			}
		}
	}
	return matches;
};

ms.familyTree.prototype.addRings = function(generation) {
	var self = this;
	var newRings = [];
	generation.forEach(function(node, index) {
		var graph = node.getGraph().copy();
		var endpoints = graph.getOuterEndpoints();
		// There are multiple endpoint windings when there are open endpoints inside a loop.
		// I think these are more trouble than they're worth. Let's skip them.
		var endpointWindings = graph.endpointWindings();
		if (endpointWindings.length > 1) {
			return;
		}
		var winding = 0;
		if (endpointWindings.length == 0) {
			var angles          = [];
			var windingAngles   = [];
			var sortedEndpoints = [];
		} else {
			var angles          = endpointWindings[0].map(function(winding) { return winding.endpoint.getAngle(); });
			var sortedEndpoints = endpointWindings[0].map(function(winding) { return winding.endpoint });
			
			// Reorder the angles.
			for (var i = 0; i < angles.length - 1; i++) {
				if (angles[i + 1] > angles[i] + ms.familyTree.windingNumberPrecision) {
					var minIndex = i;
					var maxIndex = i + 1;
					angles = angles.slice(maxIndex).concat(angles.slice(0, maxIndex));
					sortedEndpoints = sortedEndpoints.slice(maxIndex).concat(sortedEndpoints.slice(0, maxIndex));
					break;
				}
			}
			var newOrdering = graph.getOuterEndpoints().map(function(endpoint) { return sortedEndpoints.indexOf(endpoint); });
			graph.getOuterVertex().reorderEndpoints(newOrdering);
			for (var i = 0; i < sortedEndpoints.length; i++) {
				if (sortedEndpoints [i] != graph.getOuterEndpoints()[i]) {
					ms.alert('Sorting endpoints wrong.');
				}
			}
			// Compute the windingAngles using the new first endpoint.
			var endpointWindingsNew = graph.endpointWindings();
			windingAngles = endpointWindingsNew[0].map(function(winding) { return winding.winding });
		}
		var newRing = new ms.ring(angles, windingAngles, sortedEndpoints, graph);
		newRing.setNode(node);
		var matchingGroup = self.findRingMatch(newRing, newRing.getEndpointIds(), newRing.getWindingAngles());
		if (matchingGroup) {
			if (!self.ignoredIfWindingDisabled) {
				if (matchingGroup.ignoredIfWindingDisabled() || newRing.ignoredIfWindingDisabled()) {
					self.ignoredIfWindingDisabled = true;
					ms.alert('ignoredIfWindingDisabled');
				}
			}
			
			self.addMatchingGroups(matchingGroup, ms.ringGroup.createFromRing(newRing));
			node.destroy();
		} else if (ms.globalSettings.get('Match Backwards')) {
			var newMatches = self.findBackwardMatch(newRing);
			self.backwardMatches = self.backwardMatches.concat(newMatches);
		}
		self.addRing(newRing);
	});
};

ms.familyTree.addMutableRings = function(group, mutableRings) {
	var rings = group.getRings();
	for (var i = 0; i < rings.length; i++) {
		if (!mutableRings.includes(rings[i])) {
			mutableRings.push(rings[i]);
		}
	}
};

ms.familyTree.prototype.addMutableRings = function(group) {
	ms.familyTree.addMutableRings(group, this.mutableRings);
};

ms.familyTree.prototype.addMatchingGroups = function(oldGroup, newGroup) {
	var match = this.ringTransitions.find(function(transition) {
		var endpointOrder = transition.matchGroup(oldGroup);
		if (endpointOrder) {
			newGroup.endpointData = ms.familyTreeEdge.reorder(newGroup.endpointData, endpointOrder);
			newGroup.getGraph().getOuterVertex().reorderEndpoints(endpointOrder);
		}
		return !!endpointOrder;
	});
	if (match) {
		match.addGroup(newGroup);
	} else {
		this.addRingTransition(new ms.ringTransition([oldGroup, newGroup]));
	}
	this.addMutableRings(oldGroup);
	this.addMutableRings(newGroup);
};

ms.familyTree.prototype.createInitialGenerations = function(graphs) {
	this.ringTable = [];
	this.ringTransitions = [];
	this.mutableRings = [];
	this.addRing(this.emptyRing, true);
	ms.familyTreeNode.count = 0;
	ms.familyTreeEdge.count = 0;
	ms.transistorPath.count = 0;
	ms.windingTreeNode.count = 0;
	ms.ring.count = 0;
	ms.ringGroup.count = 0;
	ms.graph.count = 0;
	ms.graphEdge.count = 0;
	ms.ringTransition.count = 0;
	ms.transistorEdgeBlocker.count = 0;

	this.vertexGraphs = graphs.vertex;
	var edgeGraphs = graphs.edge;
	this.edgeToEndpointsMap = ms.familyTree.edgeToEndpointsMap(this.edgeTypes, this.vertexGraphs);

	var self = this;
	this.edgeNodes = edgeGraphs.map(function(edgeGraph) {
		var node = new ms.familyTreeNode(edgeGraph, null, 0);
		node.createChildEdges(self.edgeToEndpointsMap);
		self.addNode(node, false);
		// Destroy edges with no children. This can happen when using stubs.
		var isEmpty = !node.childEdges.some(function(childGroup) {
			// childGroup is empty for ground edges.
			return childGroup && Object.keys(childGroup).length > 0;
		});
		isEmpty && node.destroy();
		return node;
	});
	var generation1 = [];
	this.vertexGraphs.forEach(function(vertexGraph) {
		var newNode = self.addPrimaryVertexGraph(vertexGraph);
		if (newNode) {
			generation1.push(newNode);
		}
	});
	this.addStubs(generation1);
};

ms.familyTree.prototype.addPrimaryVertexGraph = function(vertexGraph) {
	var node = new ms.familyTreeNode(vertexGraph.copy(), null, 1);
	node.createChildEdges(this.edgeToEndpointsMap);
	this.addNode(node, true);

	var child = node;
	var outerEndpoints = vertexGraph.getOuterEndpoints();
	for (var j = 0; j < outerEndpoints.length; j++) {
		var parentEndpoint = outerEndpoints[j];
		if (parentEndpoint.getEdge().getCore().isGroundType()) {
			continue;
		}
		var parent = this.edgeNodes[this.edgeTypes.indexOf(parentEndpoint.getEdge().getCore())];
		var parentEndpointIndex = parentEndpoint.getIsAtStart() ? 1 : 0;
		var oppositeEdgeIndex = 1 - parentEndpointIndex;
		
		var parentDest = [null, null];
		parentDest[oppositeEdgeIndex] = child.getEndpoint(j);
		var childSource = [];
		for (var k = 0; k < outerEndpoints.length; k++) {
			if (k == j) {
				childSource[k] = parent.getEndpoint(oppositeEdgeIndex);
			} else {
				childSource[k] = outerEndpoints[k];
			}
		}
		var parentAttachment = parent.getEndpoint(parentEndpointIndex);
		var edge = new ms.familyTreeEdge(
			parent, child, [parentAttachment], parentEndpoint, parentDest, childSource);
		edge.attach();
	}
	// The node has to be destroyed a second time, because when it was first destroyed it was
	// not connected to its parent.
	if (node.isDestroyed()) {
		node.destroy();
		return null;
	}
	return node;
};

ms.familyTree.endpointAttachments = function(endpoint, edgeToEndpointsMap) {
	var edgeId = endpoint.getEdge().getCore().id;
	return edgeToEndpointsMap[edgeId][1 - endpoint.getEdgeIndex()];
};

ms.familyTree.edgeToEndpointsMap = function(edgeTypes, graphs) {
	var endpointMap = {};
	edgeTypes.forEach(function(edgeType) {
		endpointMap[edgeType.id] = [[], []];
	});
	graphs.forEach(function(graph) {
		var endpoints = graph.getOuterEndpoints();
		endpoints.forEach(function(endpoint) {
			var id = endpoint.id();
			endpointMap[id.edge][id.dir].push(endpoint);
		});
	});
	return endpointMap;
};

// For each endpoint in the graph that is in endpointMap, add a copy of the endpoint in the copy graph.
ms.familyTree.addCopyToMap = function(endpointMap, graph, copy) {
	var keys = Object.keys(endpointMap)
	for (var i0 = 0; i0 < keys.length; i0++) {
		var i = keys[i0];
		for (var j = 0; j < endpointMap[i].length; j++) {
			var endpointToCopy = null;
			for (var k = 0; k < endpointMap[i][j].length; k++) {
				if (endpointMap[i][j][k].getGraph() == graph) {
					endpointToCopy = endpointMap[i][j][k];
				}
			}
			if (endpointToCopy) {
				var endpointIndex = graph.getOuterEndpoints().indexOf(endpointToCopy);
				// Endpoints that were attached are no longer outer endpoints.
				if (endpointIndex >= 0) {
					endpointMap[i][j].push(copy.getOuterEndpoints()[endpointIndex]);
				}
			}
		}
	}
};
	
ms.familyTree.applySuperVertex = function(edgeTypes, graphs, mergeGround) {
	var endpointMap = ms.familyTree.edgeToEndpointsMap(edgeTypes, graphs);
	var removedTypes = [];
	for (var i = 0; i < edgeTypes.length; i++) {
		var id = edgeTypes[i].id;
		var mapped = endpointMap[id];
		var attachIndex = mapped.findIndex(function(m) { return m.length == 1; });
		if (attachIndex >= 0 && (mergeGround || !edgeTypes[i].isGroundType())) {
			removedTypes.push(edgeTypes[i]);
			var attachees = mapped[1 - attachIndex];
			var attacher = mapped[attachIndex][0];
			var attachers = [attacher];
			for (var k = 1; k < attachees.length; k++) {
				// If there is more than one attachee, copy the attacher including everywhere it is in endpointMap.
				attachers[k] = attacher.copy();
				graphs.push(attachers[k].getGraph());
				ms.familyTree.addCopyToMap(endpointMap, attacher.getGraph(), attachers[k].getGraph()); 
			}
			
			for (var k = 0; k < attachees.length; k++) {
				var result = attachers[k].attachEndpoint(attachees[k]);
				
				// Update the graphs. Switch the old endpoints with the new ones.
				if (result.oldGraph) {
					ms.remove(result.oldGraph, graphs);
					for (var j = 0; j < result.oldToNewEndpoints.length; j++) {
						var change = result.oldToNewEndpoints[j];
						var id = change.old.id();
						var map = endpointMap[id.edge][id.dir];
						for (var m = 0; m < map.length; m ++) {
							if (map[m].getGraph() == result.oldGraph &&
								change.new.getEdge().getGraph() /* Edges that were just removed have no graph. */) {
								map[m] = change.new;
							}
						}
					}
				}
			}
		}
	}
	for (var i = 0; i < removedTypes.length; i++) {
		ms.remove(removedTypes[i], edgeTypes);
	}
};

ms.familyTree.prototype.addStubs = function(nodes) {
	if (!ms.globalSettings.get('Stubs Enabled')) {
		return;
	}
	var self = this;
	nodes.forEach(function(node) {
		var endpoints = node.getGraph().getOuterEndpoints();
		if (endpoints.length == 1) {
			var edgeType = endpoints[0].getEdge().getCore();
			// Stubs use two fragments. Stubs are not allowed if fragments cannot be split.
			if (!edgeType.splittable()) {
				return;
			}
			var id = edgeType.id;
			var edgeIndex = endpoints[0].edgeIndex;
			var winding = ms.graph.traceToExit(endpoints[0]).winding;
			// The stub should wind pi radians otherwise it is in a loop. For example, see
			// the divided box. I'm not sure if -pi works, but I'm allowing it.
			if (!self.stubs[id][edgeIndex] && Math.abs(Math.abs(winding) - Math.PI) < 1e-5) {
				self.stubs[id][edgeIndex] = node.getGraph();
			}
		}
	});
};

ms.familyTree.prototype.createNextGeneration = function(scoreCap) {
	var self = this;
	var nextGeneration = [];
	var prevGeneration = this.leafNodes.slice();
	prevGeneration.forEach(function(node) {
		var result = node.generateChildren(self.edgeToEndpointsMap, scoreCap);
		if (result.done) {
			self.removeLeaf(node);
		}
		var newChildren = result.children;
		newChildren = newChildren.filter(function(c) {
			return !nextGeneration.includes(c);
		});
		nextGeneration = nextGeneration.concat(newChildren);
	});
	this.backwardMatches = [];
	nextGeneration.forEach(function(node) {
		self.addNode(node);
	});
	this.addStubs(nextGeneration);
	var filtered = ms.familyTree.filterBackwardMatches(this.backwardMatches);
	if (filtered.length > 0) {
		for (var i = 0; i < filtered.length; i++) {
			this.applyBackwardsRemove(filtered[i].ring);
		}
		this.applyBackwardsAdd(filtered);
		// Add the transitions.
		for (var i = 0; i < filtered.length; i++) {
			var ringGroup = ms.ringGroup.createFromRing(filtered[i].ring);	
			var transition = new ms.ringTransition([ringGroup, filtered[i].group]);
			this.addRingTransition(transition);
		}	
		for (var i = 0; i < filtered.length; i++) {
			filtered[i].ring.getNode().destroy();
		}
		this.mutableRings = this.mutableRings.filter(function(ring) {
			return !filtered.some(function(filteredA) {
				return filteredA == ring;
			})
		});
	}
	return nextGeneration.length > 0;
};

ms.familyTree.prototype.draw = function(view, offset) {
	if (this.allNodes.length == 0) {
		return;
	}
	var margin = 50;
	var highlightedSize = ms.graph.HIGHLIGHTED_SIZE;
	offset = new ms.vec2(offset.x + margin, offset.y + margin + ms.graph.HIGHLIGHTED_SIZE);
	var w = ms.mainController.getCanvasWidth() - margin;
	var h = ms.mainController.getCanvasHeight() - margin;

	var numCols = ms.familyTree.lowGenerationColumns;
	var populationSize = this.allNodes.length;
	var numCols = Math.max(ms.familyTree.lowGenerationColumns, Math.floor(Math.sqrt(populationSize)), this.ringTransitions.length); 
	var generationRows = Math.ceil(populationSize / numCols);
	var numRows = 1 + ms.familyTree.ringTransitionRows + generationRows;

	var displaySize = 85;
	offset = new ms.vec2(10, 10);
	var extraSpacing = 5;

	// Draw the transitions.
	var options = {};
	var col = 0;
	var maxColumnHeight = 0;
	var numRules = 0;
	for (var i = 0; i < this.ringTransitions.length; i++) {
		var groups = this.ringTransitions[i].getGroups();
		if (groups.length <= 1) {
			// Not sure how transition got in here with only one group.
			continue;
		}
		var row = 0;
		for (var j = 0; j < groups.length; j++) {
			var rings = groups[j].getRings();
			for (var k = 0; k < rings.length; k++) {
				var rect = [col * displaySize + offset.x, (col + 1) * displaySize + offset.x, row * displaySize + offset.y, (row + 1) * displaySize + offset.y];
				rect[0] += extraSpacing;	rect[1] -= extraSpacing;
				rect[2] += extraSpacing;	rect[3] -= extraSpacing;
				options.rect = rect;
				var minScreen = view.convertToScreen(new ms.vec2(rect[0], rect[1]));
				var maxScreen = view.convertToScreen(new ms.vec2(rect[2], rect[3]));
				options.lineWidth = 2;
				// if ((maxScreen.x > 0 && maxScreen.y > 0) && (minScreen.x < 2000 && minScreen.y < 2000)) {
					rings[k].getGraph().draw(view, options);
				// }
				row++;
			}
			if (j < groups.length - 1) {
				var arrowX = (col + 0.5) * displaySize + offset.x;
				var startY = (row + 0.05) * displaySize + offset.y;
				var endY   = (row + 0.35) * displaySize + offset.y;
				var startPos = new ms.vec2(arrowX, startY);
				var endPos   = new ms.vec2(arrowX, endY);
				view.context.lineWidth = 1;
				view.drawLine('#000', startPos, endPos, ms.vec2.ORIGIN, {hasArrows: [true, true], arrowSize: 10});
				view.context.stroke();
				row += 0.4;
			}
		}
		var columnHeight = row * displaySize;
		maxColumnHeight = Math.max(maxColumnHeight, columnHeight);
		
		col++;
		if ((col + 1) * displaySize + offset.x > w) {
			col = 0;
			offset.y += maxColumnHeight + displaySize;
			maxColumnHeight = 0;
		}
		numRules += groups.length - 1;
	}
	console.log('# of Rules: ' + numRules);
	console.log('Hierarchy Size: ' + this.allNodes.length);
	
	
	//if (ms.globalSettings.get('New Transition Layout')) {
	// return;
	// }

	// Draw the generations.
	numCols = 18;
	var margin = 5;
	var rowStart = ms.familyTree.ringTransitionRows;
	var nodes = this.allNodes;
	for (var i = 0; i < nodes.length; i++) {
		var col = i % numCols;
		var row = rowStart + Math.floor(i / numCols);
		options.rect = [col * displaySize + offset.x, (col + 1) * displaySize + offset.x, row * displaySize + offset.y, (row + 1) * displaySize + offset.y];
		options.rect[0] += margin;
		options.rect[1] -= margin;
		options.rect[2] += margin;
		options.rect[3] -= margin;
		options.color = nodes[i].isDestroyed() ? '#f0f' : '#500';
		options.colorOverride = nodes[i].isDestroyed();
		options.lineWidth = nodes[i].isDestroyed() ? 2 : 1;
		nodes[i].getGraph().draw(view, options);
	}
};

ms.familyTree.prototype.getStarterRing = function() {
	return this.emptyRing;
};
