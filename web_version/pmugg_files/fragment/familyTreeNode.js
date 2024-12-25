ms.familyTreeNode = function(graph, windingPath, cost) {
	this.parentEdges = [];
	this.childEdges = [];
	this.destroyed = false;
	this.redundant = false;
	this.graph = graph;
	this.ring = null;
	this.windingNode = null;
	this.cost = cost;
	this.ignoredIfWindingDisabled = false;
	this.id = ms.familyTreeNode.count++;
	
	if (windingPath) {
		this.windingNode = new ms.windingTreeNode(this, windingPath);
		ms.windingTreeNode.findRepetition(this.windingNode);
		var trace = ms.graph.traceToExit(windingPath.endpoint);
		if (trace.winding > -2 * Math.PI - 1e-5) {
			this.windingNode.permanent = true;
		}
	} else {
		var overlyTurned = [];
		var allWindings = graph.endpointWindings();
		allWindings.forEach(function(windings) {	
			windings.push({winding: 2 * Math.PI});
			for (var i = 0; i < windings.length - 1; i++) {
				var turning = windings[i + 1].winding - windings[i].winding - Math.PI;
				if (turning < -2 * Math.PI - 1e-5) {
					overlyTurned.push(windings[i].endpoint);
				}
			}
		});
		if (overlyTurned.length > 1) {
			// TODO: Maybe reenable.
			// ms.alert('More than one overly turned endpoint.');
		} else if (overlyTurned.length == 1) {
			var wPath = {endpoint: overlyTurned[0], parent: null, winding: 0};
			this.windingNode = new ms.windingTreeNode(this, wPath);
		}
	}
};

ms.familyTreeNode.count = 0;

// This may not be right. It would be best to use the attachIndex, but this gets
// complicated and you need to keep track of the attachment indices when connecting uncles.
ms.familyTreeNode.prototype.attachingWindingParent = function(windingParent) {
	if (!this.windingNode) {
		this.windingNode = new ms.windingTreeNode(this, {parent: windingParent});
		this.windingNode.permanent = true;
	} else {
		this.windingNode.parent = windingParent;
		windingParent.addChild(this.windingNode);
	}
};

/* ms.familyTreeNode.inLoop = function(loops, x) {
	return loops.some(function(loop) {
		return loop.includes(x);
	});
}; */

ms.familyTreeNode.prototype.createChildEdges = function(edgeToEndpointsMap, opt_recreate) {
	var loops = this.findLoops();
	var self = this;
	var outerEndpoints = this.graph.getOuterEndpoints();
	var windingEnabled = ms.globalSettings.get('Winding Enabled');
	
	for (var i = 0; i < outerEndpoints.length; i++) {
		var endpoint = outerEndpoints[i];
		if (endpoint.getEdge().getCore().isGroundType()) {
			this.childEdges[i] = null;
			continue;
		}
		// if (!windingEnabled && ms.familyTreeNode.inLoop(loops, i)) {
		if (!windingEnabled && this.windingNode) {
			this.childEdges[i] = null;
			continue;
		}
		var ignoredIfWindingDisabled = !!this.windingNode;
		var childGroup = {};
		var oldChildGroup = this.childEdges[i] || {};
		var attachments = ms.familyTree.endpointAttachments(endpoint, edgeToEndpointsMap);
		var windingParent = null;
		if (self.windingNode && self.windingNode.endpoint == endpoint) {
			windingParent = self.windingNode;
			windingParent.setNumPending(attachments.length);
		}
		attachments.forEach(function(e) {
			var eId = e.connectionId();
			if (oldChildGroup[eId]) {
				childGroup[eId] = oldChildGroup[eId];
				if (childGroup[eId] instanceof ms.familyTreePendingEdge) {
					childGroup[eId].ignoredIfWindingDisabled = ignoredIfWindingDisabled;
				} else {
					childGroup[eId].child.ignoredIfWindingDisabled = ignoredIfWindingDisabled;
				}
				if (opt_recreate) {
					childGroup[eId].destroyed = false;
					childGroup[eId].child.destroyed = false;
				}
			} else {
				childGroup[eId] = new ms.familyTreePendingEdge(
					self, [endpoint], e, edgeToEndpointsMap, loops.length);
				childGroup[eId].ignoredIfWindingDisabled = ignoredIfWindingDisabled;
			}
			if (windingParent && childGroup[eId]) {
				if (childGroup[eId].isPending()) {
					var trace = ms.graph.traceToExit(e, true);
					var attachIndex = trace.exit.outerIndex();
					childGroup[eId].setWindingPath({parent: windingParent, winding: -trace.winding, attachIndex: attachIndex});
				}
			}
		});
		this.childEdges[i] = childGroup;
	}
	if (loops.length > 0) {
		var childGroup = {};
		for (var i = 0; i < loops.length; i++) {
			var loop = loops[i];
			var id = loop.sort().join(',');
			var attachments = [outerEndpoints[loop[0]], outerEndpoints[loop[1]]];
			childGroup[id] = new ms.familyTreePendingEdge(self, attachments, null, edgeToEndpointsMap, 0);
		}
		this.childEdges[outerEndpoints.length] = childGroup;
	}
};

ms.familyTreeNode.prototype.setRing = function(ring) {
	this.ring = ring;
};

ms.familyTreeNode.prototype.getGraph = function() {
	return this.graph;
};

ms.familyTreeNode.prototype.getCost = function() {
	return this.cost;
};

ms.familyTreeNode.prototype.getEndpoint = function(index) {
	return this.graph.getOuterEndpoints()[index];
};

ms.familyTreeNode.prototype.getParentEdges = function() {
	return this.parentEdges;
};

ms.familyTreeNode.prototype.addParentEdge = function(parentEdge) {
	this.parentEdges.push(parentEdge);
};

ms.familyTreeNode.prototype.addChildEdge = function(childEdge, edgeIndex, connectionId) {
	/* if (!this.childEdges[edgeIndex].hasOwnProperty(connectionId)) {
		ms.alert('Missing child connection ID.');		
	} */
	var prevEdge = this.childEdges[edgeIndex][connectionId];
	if (prevEdge && (!prevEdge.isPending() || prevEdge.isDestroyed())) {
		ms.alert('Child edge already exists.');
	}
	this.childEdges[edgeIndex][connectionId] = childEdge;
};

ms.familyTreeNode.prototype.getChild = function(edgeIndex, connectionId) {
	return this.childEdges[edgeIndex][connectionId].child;
};

ms.familyTreeNode.prototype.getChildEdge = function(edgeIndex, connectionId) {
	if (edgeIndex == 'join') {
		edgeIndex = this.childEdges.length - 1
	}
	if (!this.childEdges[edgeIndex]) {
		// return null;
	}
	return this.childEdges[edgeIndex][connectionId];
};

ms.familyTreeNode.prototype.isDestroyed = function() {
	return this.destroyed;
};

ms.familyTreeNode.prototype.setRedundant = function(redundant) {
	this.redundant = redundant;
	this.parentEdges = [];
	this.childEdges= [];
};

ms.familyTreeNode.prototype.isRedundant = function() {
	return this.redundant;
};

ms.familyTreeNode.prototype.switchEndpointOrder = function(newOrdering) {
	this.getGraph().getOuterVertex().reorderEndpoints(newOrdering);
	this.childEdges = ms.familyTreeEdge.reorder(this.childEdges, newOrdering);
	for (var i = 0; i < this.parentEdges.length; i++) {
		this.parentEdges[i].switchChildOrder(newOrdering);
	}
};

ms.familyTreeNode.prototype.generateChildren = function(edgeToEndpointsMap, scoreCap) {
	if (this.destroyed) {
		return {children: [], done: true};
	}
	var newChildren = [];
	var addChildCallback = function(child) {
		newChildren.push(child);
	};
	var parentOuterEndpoints = this.graph.getOuterEndpoints();
	if (parentOuterEndpoints.length > scoreCap) {
		return {children: [], done: false};
	}

	var missingChildren = false;
	for (var edgeA = 0; edgeA < this.childEdges.length; edgeA++) {
		var childGroup = this.childEdges[edgeA];
		if (!childGroup) {
			continue;
		}
		var groupEdges = Object.values(childGroup);
		for (var eIndex = 0; eIndex < groupEdges.length; eIndex++) {
			var edge = groupEdges[eIndex];
			// Only generate children that haven't been generated.
			if (!edge || !edge.isPending() || edge.isDestroyed()) {
				continue;
			}
			if (edge.getScore() > scoreCap) {
				missingChildren = true;
				continue;
			}
			edge.create(addChildCallback);
		}
	}
	newChildren = newChildren.filter(function(child) {
		return !child.isRedundant() && !child.isDestroyed();
	});
	return {children: newChildren, done: !missingChildren};
};

ms.familyTreeNode.Status = {
	DESTROY: 0,
	CONTINUE: 1,
	SUCCESS: 2,
};


ms.familyTreeNode.SourceType = {
	A: 'A',
	B: 'B',
	ORIGINAL: 'O', // Original means it came from the grandparent.
};

// C is the child. 
// P is the parent.
// G is the grandparent.
// U is the uncle.
// Edges PC and GU adds vertex A.
// Edges GP and UC adds vertex B.
ms.familyTreeNode.connectUncles = function(edgePC, addChildCallback) {
	var Status = ms.familyTreeNode.Status;
	var newEdgesUC = [];
	var parent = edgePC.parent;
	var edgeA = edgePC.parentEdgeIndex();
	var connectionA = edgePC.connectionId();
	for (var i = 0; i < parent.parentEdges.length; i++) {		
		var edgeGP = parent.parentEdges[i];
		var uncleProps = ms.familyTreeNode.findUncleProps(edgePC, edgeGP, addChildCallback);

		if (uncleProps.status == Status.FAIL) {			
			edgePC.child.destroy();
			return;
		}
		if (uncleProps.status == Status.CONTINUE) {
			continue;
		}
		var edgeIndexUC = ms.familyTreeEdge.parentEdgeIndex(uncleProps.removalsUC);
		var connectionUC = ms.familyTreeEdge.connectionId(uncleProps.removalsUC, uncleProps.attachment);
		var edgeUC = uncleProps.edgeGU.child.getChildEdge(edgeIndexUC, connectionUC);
		// if (edgeUC === null) {
		if (!edgeUC || edgeUC.isDestroyed()) {
			edgePC.child.destroy();
			return;
		}
		if (edgeUC.isPending()) {
			ms.familyTreeNode.connectUncle(edgePC, edgeGP, uncleProps, addChildCallback);
			newEdgesUC.push(uncleProps.edgeGU.child.getChildEdge(edgeIndexUC, connectionUC));
		}
	}
	// edgePC.child.graph.isMonotonic();

	newEdgesUC.forEach(function(newEdge) {
		ms.familyTreeNode.connectUncles(newEdge, addChildCallback);
	});
};

ms.familyTreeNode.findUncleProps = function(edgePC, edgeGP, addChildCallback) {
	var Status = ms.familyTreeNode.Status;
	// Find any endpoints that were added in GP and removed in PC.
	// Switching the order is more difficult.
	var gRemovals = edgePC.removals.map(function(p) { return edgeGP.childToParentEndpoint(p); });
	var G = edgeGP.parent;
	var wasAdded = gRemovals.map(function(e) { return !edgeGP.fromParent(e); });
	var addIndex = wasAdded.findIndex(function(w) { return w; });
	if (addIndex >= 0) {
		// Both endpoints could be added at one in the case of a super vertex. The house is one example.
		if (wasAdded[0] && wasAdded[1]) {
			return { status: Status.CONTINUE };
		}
		// The order of operations cannot be changed unless edgePC is a join.
		// You cannot remove something before it was added.
		if (!edgePC.isJoined()) {
			return { status: Status.CONTINUE };
		}
		var eAdded = gRemovals[addIndex];
		var eInG   = gRemovals[1 - addIndex];
		
		var connectionId = eAdded.connectionId();
		var edgeIndex = eInG.outerIndex();
		edgeGU = G.getChildEdge(edgeIndex, connectionId);
		if (!edgeGU || edgeGU.isDestroyed()) {
			return { status: Status.CONTINUE };
		}
		if (edgeGU && edgeGU.isPending()) {
			edgeGU = edgeGU.create(addChildCallback);
		}

		var addedGU = edgeGU.addedSources();
		var addedGP = edgeGP.addedSources();
		
		var specialUPmatch = addedGU.map(function(gu) {
			if (!gu) {
				return {type: 'not added'};
			}
			var parentMatch = addedGP.find(function(gp) { return gp && (gu.source == gp.source); });
			if (parentMatch) {
				return {type: 'added to both', match: parentMatch.endpoint};
			} else {
				return {type: 'added to uncle'};
			}
		});
		var addedToUncle = addedGU.filter(function(gu, index) {
			return specialUPmatch[index].type == 'added to uncle';
		});
		if (addedToUncle.length != 1) {
			ms.alert('Only one endpoint should be added to the uncle that is not in the parent.');
		}
		var removalsUC = [
			edgeGU.parentToChildEndpoint(edgeGP.removals[0]),
			addedToUncle[0].endpoint,
		];
		return { edgeGU: edgeGU, removalsUC: removalsUC, attachment: null, specialUPmatch: specialUPmatch, status: Status.SUCCESS };
	} else {
		var edgeA = edgePC.parentEdgeIndex();
		var connectionA = edgePC.connectionId();
		if (edgePC.isJoined()) {
			// Map endpoint from parent to grandparent.
			var edgeAi =  G.childEdges.length - 1;
			connectionA = connectionA.split(',').map(function(pIndex) { return edgeGP.mapChildToParent(pIndex); }).sort().join(',');
		} else {
			var edgeAi = edgeGP.mapChildToParent(edgeA);	
		}
		var edgeGU = G.getChildEdge(edgeAi, connectionA);	
		if (edgeGU && !edgeGU.isDestroyed() && edgeGU.isPending()) {
			edgeGU = edgeGU.create(addChildCallback);
		}
		if (!edgeGU || edgeGU.isDestroyed() || edgeGU.child.isDestroyed()) {
			return { status: Status.FAIL };
		}
		var removalsUC = edgeGP.removals.map(function(gEndpoint) {
			return edgeGU.parentToChildEndpoint(gEndpoint);
		});
		return { edgeGU: edgeGU, removalsUC: removalsUC, attachment: edgeGP.attachment, status: Status.SUCCESS };
	}
};

// C is the child. 
// P is the parent.
// G is the grandparent.
// U is the uncle.
// Edges PC and GU adds vertex A.
// Edges GP and UC adds vertex B.
ms.familyTreeNode.connectUncle = function(edgePC, edgeGP, uncleProps, addChildCallback) {
	var connectionB = edgeGP.connectionId();
	var edgeGU = uncleProps.edgeGU;

	/* var edgeB = edgeGU.mapParentToChild(edgeGP.parentEdgeIndex());
	// If edgeB does not appear in the uncle, this was removed by looping.
	// I'm not sure this is the right behavior. Earlier it didn't destroy. It returned false.
	if (edgeB == -1) {
		ms.alert('Is this still happening?');	
		edgePC.child.destroy();
		return true;
	} */
	
	// Trace the source from child to parent.
	var SourceType = ms.familyTreeNode.SourceType;
	var cSources = edgePC.childSource.map(function(pEndpoint) {
		var type = edgePC.fromParent(pEndpoint) ? SourceType.ORIGINAL : SourceType.A;
		return { type: type, endpoint: pEndpoint };
	});
	// Continue tracing from parent to grandparent.
	cSources = cSources.map(function(pSource) {
		// Ignore those introduced by source A since we've found their source.
		if (pSource.type == SourceType.A) {
			return pSource;
		}
		var gEndpoint = edgeGP.endpointSource(pSource.endpoint);
		var type = edgeGP.fromParent(gEndpoint) ? SourceType.ORIGINAL : SourceType.B;
		return { type: type, endpoint: gEndpoint };
	});
	
	// Trace the source from uncle to grandparent.
	var uSources = edgeGU.childSource.map(function(uEndpoint) {
		var type = edgeGU.fromParent(uEndpoint) ? SourceType.ORIGINAL : SourceType.A;
		return { type: type, endpoint: uEndpoint };
	});
	var mapUC = uSources.map(function(uSource) {
		return cSources.findIndex(function(cSource) {
			return uSource.type == cSource.type && uSource.endpoint == cSource.endpoint;
		});
	});
	var GPmatch = uncleProps.specialUPmatch;
	if (GPmatch) {
		for (var j = 0; j < GPmatch.length; j++) {			
			if (GPmatch[j].type == 'added to both') {
				var parentIndex = GPmatch[j].match.outerIndex();
				mapUC[j] = edgePC.parentDest[parentIndex].outerIndex();
			}
		}
	}

	var uncle = edgeGU.child;
	var child = edgePC.child;		
	var childEdgeLength = child.getGraph().getOuterEndpoints().length;
	var uncleEdgeLength = uncle.getGraph().getOuterEndpoints().length;
	
	var childSourceUC = new Array(childEdgeLength).fill(null);
	var parentDestUC = new Array(uncleEdgeLength).fill(null);
	for (var j = 0; j < cSources.length; j++) {
		if (cSources[j].type == SourceType.B) {
			childSourceUC[j] = cSources[j].endpoint;
		}
	}
	for (var j = 0; j < mapUC.length; j++) {
		if (mapUC[j] >= 0) {
			var childEndpoint = child.getEndpoint(mapUC[j]);
			parentDestUC[j] = childEndpoint;
			childSourceUC[childEndpoint.outerIndex()] = uncle.getEndpoint(j);
		}
	}
	// The uncle may have a loop that the child does not have. In this case, do not propagate.
	if (childSourceUC.includes(null)) {
		// This shouldn't happening anymore.
		ms.alert('A loop can be formed in two different ways.');
		return false;
	}
	
	/* if (uncle.getChildEdge(edgeB, connectionB) === undefined) {
		ms.alert('Is this still happening?');
		child.destroy();
		return true;
	} */
	
	var edgeUC = new ms.familyTreeEdge(uncle, child, uncleProps.removalsUC, uncleProps.attachment, parentDestUC, childSourceUC);
	edgeUC.attach();
};
	

ms.familyTreeNode.prototype.merge = function(nodeB) {
	if (this == nodeB) {
		return;
	}
	if (!this.graph.possiblyEquivalent(nodeB.getGraph())) {
		ms.alert('The graphs we are merging are different.');
	}
	if (nodeB.childEdges.length != this.childEdges.length) {
		ms.alert('The childEdges are not equal.');
	}
	for (var i = 0; i < this.childEdges.length; i++) {
		if (Object.values(this.childEdges[i]).some(function(c) { return !!c && !c.isPending(); })) {
			ms.alert('Nodes are not expected to have any childEdges when merging.');
		}
		if (Object.values(nodeB.childEdges[i]).some(function(c) { return !!c && !c.isPending(); })) {
			ms.alert('Nodes are not expected to have any childEdges when merging.');
		}
	}

	var parentsB = nodeB.getParentEdges();
	for (var i = 0; i < parentsB.length; i++) {
		parentsB[i].setChild(this);
		this.addParentEdge(parentsB[i]);
	}
	nodeB.setRedundant(this);
	var b = nodeB.id;
};

ms.familyTreeNode.prototype.destroy = function() {
	this.destroyed = true;
	this.parentEdges.forEach(function(edge) {
		edge.destroy();
	});
	this.childEdges.forEach(function(childGroup) {
		childGroup && childGroup && Object.values(childGroup).forEach(function(edge) {
			edge && !edge.isDestroyed() && edge.destroy();
		});
	});
	if (this.windingNode) {
		this.windingNode.destroy0();
	}
};

ms.familyTreeNode.prototype.findLoops = function() {
	var endpointWindings = this.graph.endpointWindings();
	var self = this;
	var loops = [];
	endpointWindings.forEach(function(windings) {
		for (var a = 0; a < windings.length; a++) {
			for (var b = a + 1; b < windings.length; b++) {
				var aw = windings[a].winding;
				var bw = windings[b].winding;
				// The loop is formed from endpoint B and the twin of endpoint A.
				// Because we are dealing with A's twin we have aw + Math.PI.
				if (Math.abs(Math.abs(aw + Math.PI - bw) - 2 * Math.PI) < 1e-5) {
					var endpointA = windings[a].endpoint;
					var endpointB = windings[b].endpoint;
					// Check that it also turns 360 degress in the opposite direction.
					if (Math.abs(Math.abs(ms.graph.reverseWinding(endpointB, endpointA) - Math.PI) - 2 * Math.PI) < 1e-5 &&
						endpointA.getEdge().getCore() == endpointB.getEdge().getCore() &&
						endpointA.getEdge().getCore().isLoopy()
						// && !endpointA.getEdge().getCore().isGroundType()
						) {
						if (aw > bw) {
							loops.push([endpointA.outerIndex(), endpointB.outerIndex()]);
						} else {
							loops.push([endpointB.outerIndex(), endpointA.outerIndex()]);
						}
					}
				}
			}
		}
	})
	return loops;
};

// Check if this node is a descendant of nodeB.
ms.familyTreeNode.prototype.isDescendant = function(nodeB) {
	var ancestors = [];
	var unchecked = [this];
	while (unchecked.length > 0) {
		var toCheck = unchecked.shift();
		var parents = toCheck.getParentEdges().map(function(edge) { return edge.parent; });
		for (var i = 0; i < parents.length; i++) {
			var parent = parents[i];
			if (parent == nodeB) {
				return true;
			}
			if (!ancestors.includes(parent)) {
				ancestors.push(parent);
				unchecked.push(parent);
			}
		}
	}
	return false;
};

ms.familyTreeNode.prototype.highlight = function(view, opt_offset) {
	this.graph.highlight(view, opt_offset);
};

ms.familyTreeNode.prototype.print = function() {
	ms.highlight(this);
};

ms.familyTreeNode.prototype.printEndpoint = function(index) {
	var endpoints = this.graph.getOuterEndpoints();
	if (endpoints.length > index) {
		ms.highlight(endpoints[index]);
	}
};
