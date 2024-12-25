ms.familyTreeEdge = function(parent, child, removals, attachment, parentDest, childSource) {
	this.parent = parent;
	this.child = child;
	this.destroyed = false;

	// The outer endpoints in the parent that are not in the child because they are
	// attached to something. If there are two removals, the endpoints are attached
	// to themselves by an edge. Otherwise, there is one attached to the attachment.
	this.removals = removals;
	this.attachment = attachment;
	
	// Where the parent endpoints end up in the child.
	this.parentDest = parentDest;
	// Where the child endpoints came from.
	this.childSource = childSource;

	this.id = ms.familyTreeEdge.count++;
};

ms.familyTreeEdge.count = 0;

ms.familyTreeEdge.prototype.attach = function() {
	this.child.addParentEdge(this);
	this.parent.addChildEdge(this, this.parentEdgeIndex(), this.connectionId());
};

ms.familyTreeEdge.prototype.isPending = function() {
	return false;
};

ms.familyTreeEdge.prototype.isJoined = function() {
	return !this.attachment;
};

ms.familyTreeEdge.prototype.isDestroyed = function() {
	return this.destroyed;
};

// Get the source for a child endpoint.
ms.familyTreeEdge.prototype.endpointSource = function(endpoint) {
	return this.childSource[endpoint.outerIndex()];
};

ms.familyTreeEdge.parentEdgeIndex = function(removals) {
	if (removals.length == 2) {
		return 'join';
	} else {
		return removals[0].outerIndex();
	}
};

ms.familyTreeEdge.connectionId = function(removals, attachment) {
	if (attachment) {
		return attachment.connectionId();
	} else {
		return removals.map(function(e) { return e.outerIndex(); }).sort().join(',');
	}
};

ms.familyTreeEdge.prototype.parentEdgeIndex = function() {
	var edgeIndex = ms.familyTreeEdge.parentEdgeIndex(this.removals);
	if (edgeIndex == 'join') {
		return this.parent.childEdges.length - 1;
	} else {
		return edgeIndex;
	}
};

ms.familyTreeEdge.prototype.connectionId = function() {
	return ms.familyTreeEdge.connectionId(this.removals, this.attachment);
};

ms.familyTreeEdge.prototype.destroy = function() {
	if (this.parent.isDestroyed() && !this.child.isDestroyed()) {
		this.child.destroy();
	}
	if (this.child.isDestroyed() && !this.parent.isDestroyed()) {
		var edgeIndex = this.parentEdgeIndex();
		var edges = this.parent.childEdges[edgeIndex];
		// delete edges[this.connectionId()];
		if (edges[this.connectionId()]) {
			edges[this.connectionId()].destroyed = true;
		}
		var nonNull = Object.values(edges).filter(function(value) { return value && !value.isDestroyed(); });
		// TODO: The join stuff may need to be reexamined.
		if (nonNull.length == 0 && !this.isJoined()) {
			this.parent.destroy();
		}
	}
	if (!this.child.isDestroyed() && !this.parent.isDestroyed()) {
		ms.alert('Something should be destroyed.');
	}
};

// If the child is null, we are creating one.
ms.familyTreeEdge.create = function(parent, windingPath, addChildCallback, removals, attachment, edgeToEndpointsMap, ignoredIfWindingDisabled) {
	// Return if this edge has already been destroyed.
	// if (!parent.childEdges[edgeIndex].hasOwnProperty(attachmentEndpoint.connectionId())) {
	// 	return;
	// }
	
	var attachments;
	if (attachment) {
		attachments = [attachment].concat(removals);
	} else {
		attachments = removals;
	}
	
	var attachedResult = ms.graphEndpoint.attachEndpoints(attachments[0], attachments[1]);
	var childGraph = attachedResult.newGraph;
	var parentDest = attachedResult.bDest;
	var childSource = attachedResult.newSource;
	if (windingPath) {
		windingPath.endpoint = attachedResult.aDest[windingPath.attachIndex];
		// If we reach a dead end continue on with the next leftmost / rightmost endpoint.
		if (!windingPath.endpoint) {
			var index = ms.graph.traceToExit(attachments[1], windingPath.winding < 0).exit.outerIndex();
			windingPath.endpoint = parentDest[index];
		}
	}

	var child = new ms.familyTreeNode(childGraph, windingPath, parent.cost + 1);
	child.ignoredIfWindingDisabled = ignoredIfWindingDisabled;
	child.createChildEdges(edgeToEndpointsMap);
	addChildCallback(child);	
	
	var edge = new ms.familyTreeEdge(parent, child, removals, attachment, parentDest, childSource);

	/* if (childOverride.isRedundant()) {
		childOverride = childOverride.isRedundant();
	}
	child = childOverride;
	edge.setChild(childOverride); */

	edge.attach();
	if (!ms.globalSettings.get('Winding Enabled') && child.windingNode) {
		child.destroy();
		return edge;
	}
	
	ms.familyTreeNode.connectUncles(edge, addChildCallback, edgeToEndpointsMap);

	return edge;
};

// Returns true if the endpoint is in the parent graph.
ms.familyTreeEdge.prototype.fromParent = function(endpoint) {
	return this.parent.getGraph() == endpoint.getGraph();
};

// Map the endpoint index from the child to the parent.
ms.familyTreeEdge.prototype.mapChildToParent = function(index) {
	var source = this.childSource[index]
	return this.fromParent(source) ? source.outerIndex() : -1;
};

// Map the endpoint index from the child to the parent.
ms.familyTreeEdge.prototype.mapParentToChild = function(index) {
	var dest = this.parentDest[index]
	return dest ? dest.outerIndex() : -1;
};

// Map the endpoint from the child to the parent.
ms.familyTreeEdge.prototype.childToParentEndpoint = function(endpoint) {
	return this.childSource[endpoint.outerIndex()];
};

// Map the endpoint from the child to the parent.
ms.familyTreeEdge.prototype.parentToChildEndpoint = function(endpoint) {
	return this.parentDest[endpoint.outerIndex()];
};

// Returns the endpoints and sources that were added to the child, not found in the parent.
ms.familyTreeEdge.prototype.addedSources = function() {
	var added = [];
	var outerEndpoints = this.child.getGraph().getOuterEndpoints();
	for (var i = 0; i < this.childSource.length; i++) {
		var source = this.childSource[i];
		if (!this.fromParent(source)) {
			added.push({endpoint: outerEndpoints[i], source: source});
		} else {
			added.push(null);
		}
	}
	return added;
};

// n is the number of elements in the inverse.
ms.familyTreeEdge.invertMapping = function(mapping, n) {
	var inverse = new Array(n);
	inverse.fill(-1);
	for (var i = 0; i < mapping.length; i++) {
		if (mapping[i] >= 0) {
			inverse[mapping[i]] = i;
		}
	}
	return inverse;
};

ms.familyTreeEdge.reorder = function(array, newOrdering) {
	if (array.length != newOrdering.length) {
		ms.alert('Order array is not the same size in reorder.');
	}
	var result = [];
	for (var i = 0; i < newOrdering.length; i++) {
		result[newOrdering[i]] = array[i];
	}
	return result;
};

ms.familyTreeEdge.prototype.highlight = function(view) {
	var optionsP = { offset: new ms.vec2(30, 30)};
	var optionsC = { offset: new ms.vec2(130, 30)};
	this.parent.highlight(view, optionsP);
	this.child.highlight(view, optionsC);
};

ms.familyTreeEdge.prototype.print = function() {
	ms.highlight(this);
};
