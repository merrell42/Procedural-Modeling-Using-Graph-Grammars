ms.shape3D = function(vertexTypes, edgeTypes, faceTypes, xml) {
	this.vertexTypes = vertexTypes;
	this.edgeTypes = edgeTypes;
	this.splicedEdgeTypes = {};
	this.faceTypes = faceTypes;
	this.is3D = true;
	this.xml = xml;
};

ms.shape3D.prototype.export = function() {
	var exp = ((x) => x.export(this));
	return {
		vertexTypes: this.vertexTypes.map(exp),
		edgeTypes: this.edgeTypes.map(exp),
		faceTypes: this.faceTypes.map(exp),
		xml: this.xml,
	};
};

ms.shape3D.import = function(json) {
	var result = new ms.shape3D([], [], [], json.xml);
	result.faceTypes = json.faceTypes.map((type) => ms.faceType3D.import(type));
	result.edgeTypes = json.edgeTypes.map((type) => ms.edgeType3D.import(type, result));
	result.vertexTypes = json.vertexTypes.map((type) => ms.vertexType.import(type, result));
	return result;
};

ms.shape3D.prototype.setNetworks = function(networks) {
	this.chainMap = networks.chainMap;
	this.vertex = networks.vertex;
	this.edge = networks.edge;
	this.face = networks.face;
};

ms.shape3D.prototype.getShape = function() {
	return this;
};

ms.shape3D.prototype.getVertices = function() {
	return [0];
};

ms.shape3D.prototype.getSpliceEdgeType = function(faceType, dir) {
	return ms.shape3D.getSpliceEdgeType(this.splicedEdgeTypes, this.edgeTypes, faceType, dir);
};

ms.shape3D.getSpliceEdgeType = function(splicedEdgeTypes, edgeTypes, faceType, dir) {
	var key = faceType.id + ',' + dir;
	if (!splicedEdgeTypes[key]) {
		var faceData = [{type: faceType, onRight: true}, {type: faceType, onRight: false}]
		var uv = (dir == 0) ? faceType.u : faceType.v;
		var edgeType = new ms.edgeType3D(faceData, uv);
		edgeType.setSpliced(true);
		splicedEdgeTypes[key] = edgeType;
		edgeTypes.push(edgeType);
	}
	return splicedEdgeTypes[key];
};


ms.shapeFile3D = function(name) {
	this.name = name;
	this.is3D = true;
};

ms.shapeFile3D.prototype.getName = function() {
	return this.name;
};

ms.shapeFile3D.prototype.isEmpty = function() {
	return false;
};

ms.shapeFile3D.prototype.parse = function() {
	var objFile = new OBJFile(ms.obj[this.name].obj, this.name);
	objFile.parse();
	this.model = objFile.result.models[0];


	// Switch from 1-index to 0-index.
	this.model.faces.forEach(function(face) {
		var n = face.vertices.length;
		for (var i = 0; i < n; i++) {
			// The indices start with 1.
			face.vertices[i].vertexIndex--;
			face.vertices[i].textureCoordsIndex--;
			face.vertices[i].vertexNormalIndex--;
		}
	});
};
