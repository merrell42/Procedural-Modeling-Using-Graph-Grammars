// Define a group of faces. The first face is the outer component. The
// other faces are inner components and represent holes in the face.
ms.faceGroup = function(stats) {
	var properties = {
		face: new ms.alternativeArray('face', true),
	};
	this.node = new ms.node(this, stats, 'faceGroup', properties);
};

ms.faceGroup.prototype.getNode = function() {
	return this.node;
};

ms.faceGroup.prototype.getFaces = function() {
	return this.node.get('face');
};

// Connect a new face as an inner component or hole to this one.
ms.faceGroup.prototype.connectHole = function(groupB) {
	if (groupB == this) {
		// This face has already been added.
		return;
	}
	var facesB = groupB.getFaces();
	if (facesB.length != 1) {
		ms.alert('Hole group should have one face');
	}
	var faceB = facesB[0];
	faceB.getNode().disconnect(groupB);
	faceB.getNode().connect(this);
};

// ms.faceGroup.prototype.print = function() {	ms.highlight(this); };
