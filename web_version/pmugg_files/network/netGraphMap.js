// This is similar to netMorphism.
ms.netGraphMap = function() {
	this.vertexBtoA = null;
	this.edgeBtoA = null;
};

ms.netGraphMap.create = function(info) {
	var { netB } = info;
	var interiorB = netB.getInterior();

	var map = new ms.netGraphMap();
	map.vertexBtoA = new Array(interiorB.getVertices().length).fill(null);
	map.edgeBtoA   = new Array(interiorB.getEdges()   .length).fill(null);
	return map ;
};

ms.netGraphMap.prototype.copy = function() {
	var result = new ms.netGraphMap();
	result.vertexBtoA = this.vertexBtoA.slice();
	result.edgeBtoA   = this.edgeBtoA.slice();
	return result;
};
