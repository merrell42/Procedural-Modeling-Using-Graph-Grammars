ms.familyTreePendingEdge = function(parent, removals, attachment, edgeToEndpointsMap, numParentLoops) {
	this.parent = parent;
	this.windingPath = null;
	this.removals = removals;
	this.attachment = attachment;
	this.edgeToEndpointsMap = edgeToEndpointsMap;
	this.destroyed = false;
	this.numParentLoops = numParentLoops;
	this.updateScore();
	this.ignoredIfWindingDisabled = false;
	this.id = ms.familyTreeEdge.count++;
};

ms.familyTreePendingEdge.prototype.updateScore = function() {
	var parentOuterEndpoints = this.parent.graph.getOuterEndpoints();
	var extraEndpoints = this.attachment ? this.attachment.getGraph().getOuterEndpoints().length : 0;
	var numEndpoints = parentOuterEndpoints.length + extraEndpoints - 2;
	var windingPenalty = this.numParentLoops * ms.globalSettings.get('Winding Penalty');
	var windingPenaltyNew = 0;
	if (this.parent.windingNode && !this.windingPath) {
		windingPenaltyNew = ms.globalSettings.get('Winding Penalty New');
	}
	this.score = windingPenalty + windingPenaltyNew + numEndpoints;
};

ms.familyTreePendingEdge.prototype.setWindingPath = function(windingPath) {
	this.windingPath = windingPath;
	this.updateScore();
};

ms.familyTreePendingEdge.prototype.isDestroyed = function() {
	return this.destroyed;
};

ms.familyTreePendingEdge.prototype.isPending = function() {
	return true;
};

ms.familyTreePendingEdge.prototype.getScore = function() {
	return this.score;
};

ms.familyTreePendingEdge.prototype.create = function(addChildCallback) {
	if (this.destroyed) {
		ms.alert('Pending edge should be destroyed.');
	}
	return ms.familyTreeEdge.create(this.parent, this.windingPath, addChildCallback, this.removals, this.attachment, this.edgeToEndpointsMap, this.ignoredIfWindingDisabled);
};

ms.familyTreePendingEdge.prototype.destroy = function() {
	this.destroyed = true;
};
