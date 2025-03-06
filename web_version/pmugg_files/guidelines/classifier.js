ms.classifier = function() {
	this.edges = []; // this.edges is never needed.

	this.resetHierarchy();
	
	this.boundaryGroups = null;
	this.fullyConnected = false;
	this.is3D = false;
	this.xml = null;
};

ms.classifier.prototype.importSolution = function(solution) {
	if (solution.matches) {
		if (solution.xml) {
			var decorationText = solution.xml;
			var xml = ms.util.xmlTextToJson(decorationText);
			xml.decorationText = decorationText;
			this.xml = xml;
		}
		this.hierarchy = ms.networkHierarchy.partialImport(solution, xml);
	} else if (solution.useNetworks) {
		this.hierarchy = ms.networkHierarchy.import(solution);

		this.hierarchy.transitions.forEach((transition) => {
			transition.networks.forEach((network) => {
				network.morphism = network.getBoundaryMorphism();
				console.log(network.indices);
				
				// var indices = network.getConnectors().map((connector) => {
					// var vertex = connector.interior[0];
					// return vertex.network.vertices.indexOf(vertex);
				// });
				// console.log(indices);
			});
		});
		console.log(JSON.stringify(solution).replaceAll('\\"', '"').replaceAll('"{', '{').replaceAll('}"', '}'));
	} else {
		this.hierarchy = ms.familyTree.import(solution);
	}
};

ms.classifier.computeBoundaryMorphisms = function(solution) {
	if (solution.matches) {
		ms.alert('Not Supported');
	} else if (solution.useNetworks) {
		var hierarchy = ms.networkHierarchy.import(solution);
		hierarchy.transitions.forEach((transition, index) => {
			var sTransition = solution.transitions[index];
			transition.networks.forEach((network, i) => {
				sTransition.n[i].morphism = network.getBoundaryMorphism();
			});
		});
		hierarchy.starterTransitions.forEach((transition, index) => {
			var sTransition = solution.starterTransitions[index];
			transition.networks.forEach((network, i) => {
				sTransition.n[i].morphism = network.getBoundaryMorphism();
			});
		});
		hierarchy.groundTransitions.forEach((transition, index) => {
			var sTransition = solution.groundTransitions[index];
			transition.networks.forEach((network, i) => {
				sTransition.n[i].morphism = network.getBoundaryMorphism();
			});
		});
	} else {
		ms.alert('Not Supported');
	}
};

ms.classifier.prototype.resetHierarchy = function() {
	if (ms.globalSettings.get('Use Network')) {
		this.hierarchy = new ms.networkHierarchy();
	} else {
		this.hierarchy = new ms.familyTree();
	}
};

// The maximum angle difference for an edge to be classified as the same edge type.
ms.classifier.ANGLE_TOLERANCE = 10 / 180 * Math.PI;

// The angle at which bidirectional edges switch directions.
ms.classifier.FLIP_ANGLE = 53 / 180 * Math.PI;

ms.classifier.findMatchingEdge = function(edgeDatumA, edgeDataB, isRigidFixed) {	
	var matchingAreas = function(a, b) {
		return a.leftArea == b.leftArea && a.rightArea == b.rightArea;
	}
	if (isRigidFixed) {
		return edgeDataB.find(function(edgeDatumB) {
			if (!matchingAreas(edgeDatumA, edgeDatumB)) {
				return false;
			}
			if (edgeDatumA.shapeEdge == edgeDatumB.shapeEdge) {
				return true;
			}
			var shapeA = edgeDatumA.shapeEdge.shape;
			var shapeB = edgeDatumB.shapeEdge.shape;
			if (shapeA.getGroup() != shapeB.getGroup()) {
				return false;
			}
			var startA = edgeDatumA.shapeEdge.start.vertex;
			var startB = edgeDatumB.shapeEdge.start.vertex;
			var endA = edgeDatumA.shapeEdge.end.vertex;
			var endB = edgeDatumB.shapeEdge.end.vertex;
			return shapeA.getVertexIndex(startA) == shapeB.getVertexIndex(startB) &&
						 shapeA.getVertexIndex(endA) == shapeB.getVertexIndex(endB);
		});
	} else {
		return edgeDataB.find(function(edgeDatumB) {
			if (edgeDatumA.brush != edgeDatumB.brush || !matchingAreas(edgeDatumA, edgeDatumB)) {
				return false;
			}
			var classified = edgeDatumA.brush ? edgeDatumA.brush.get('Classified by angle') : true;
			if (classified) {
				return (Math.abs(ms.util.angleDifference(edgeDatumA.angle, edgeDatumB.angle)) < ms.classifier.ANGLE_TOLERANCE);
			} else {
				return true;
			}
		});
	}
};

ms.classifier.prototype.getStarterRing = function() {
	return this.hierarchy.getStarterRing();
};

ms.classifier.prototype.getEmptyRing = function() {
	return this.hierarchy.getEmptyRing();
};

ms.classifier.snapEdgeAngles = function(edgeTypes) {
	for (var a = 0; a < edgeTypes.length; a++) {
		var angleA = edgeTypes[a].getAngle();
		for (var b = a + 1; b < edgeTypes.length; b++) {
			var typeB = edgeTypes[b];
			var angleB = typeB.getAngle();
			var diff = ms.util.angleDifference(angleA, angleB);
			if (Math.abs(diff) < 2 * 2 * Math.PI * ms.familyTree.windingNumberPrecision) {
				typeB.setAngle(angleA);
			}
		}
	}
};

ms.classifier.switchDirections = function(edge) {
	var startVertex = edge.getStart().getVertex().getGroup();
	var endVertex = edge.getEnd().getVertex().getGroup();
	var angle = ms.classifier.edgeAngle(startVertex, endVertex);
	var brush = edge.getBrush();

	var face0 = edge.getEnd().getFace();
	var face1 = edge.getStart().getFace();
	var area0 = face0 ? face0.getArea() : null;
	var area1 = face1 ? face1.getArea() : null;
	if (area0 && area0.getColor() == '#fff') { area0 = null; }
	if (area1 && area1.getColor() == '#fff') { area1 = null; }
	var bidirectional = brush ? brush.get('Bidirectional Edges') : true;
	if (!bidirectional) {
		return false;
	}
	
	if (area0 == area1) {
		var flipAngle = ms.classifier.FLIP_ANGLE;
		if (ms.util.angleDifference(angle, -flipAngle) < ms.classifier.ANGLE_TOLERANCE) {
			// window.console.log('Angle is near flip angle.')
		}
		
		// Flip the start and end vertices when the angle goes past the flip angle.
		return bidirectional && (angle <= -flipAngle) || (angle > -flipAngle + Math.PI)
	} else {
		var areaId0 = area0 ? area0.getId() : -1;
		var areaId1 = area1 ? area1.getId() : -1;
		return (areaId0 > areaId1);
	}
};

ms.classifier.extractPrimitives2D = function(shapes) {
	// TODO: Clean this up and the shapes0 part.
	// Copy the shapes because something gets screwed up in the fill ground part.
	/* var shapes = shapes0.map(function(shape) {
		return shape.copy();
	}); */

	// A list of all the vertices that are at unique positions.
	var uniqueVertexData = [];
	// A map from a shape vertex to its vertex data. Several shape vertices can share a vertex datum.
	var vertexDataMap = {};
	for (var i = 0; i < shapes.length; i++) {
		var s = shapes[i];
		// Find any matching vertices at the same position. Add a new vertex if no matching vertices.
		s.getVertices().forEach(function(vertex) {
			var position = vertex.getPosition().copy();
			var matchingData = uniqueVertexData.find(function(vertexDatum) {
				return (position.distance2(vertexDatum.position, position) < 1);
			});		
			if (matchingData) {
				vertexDataMap[i + ',' + vertex.id] = matchingData;
			} else {
				var decoration = vertex.getDecoration();
				var vertexType = new ms.vertexType(decoration);
				var vertexDatum = {position: position, vertexType: vertexType, edges: []};
				uniqueVertexData.push(vertexDatum);
				vertexDataMap[i + ',' + vertex.id] = vertexDatum;
			}
		});
	}
	var blankFace = new ms.faceType3D(null, ms.vec3.Z_HAT);
	var faceTypes = [blankFace];
	var getFaceType = function(area) {
		if (area) {
			return area.getFaceType();
		} else {
			return blankFace;
		}
	};

	// A unique set of edges.
	var uniqueEdgeData = [];
	// Points from each edge to an edge datum.
	var edgeDataMap = {};
	var cellWidth = ms.globalSettings.get('Major Grid Spacing');
	for (var i = 0; i < shapes.length; i++) {
		var s = shapes[i];
		s.getEdges().forEach(function(edge) {
			var startVertex = edge.getStart().getVertex().getGroup();
			var endVertex = edge.getEnd().getVertex().getGroup();
			var angle = ms.classifier.edgeAngle(startVertex, endVertex);
			var brush = edge.getBrush();
			var isRigid = brush && brush.get('Rigid');
			var isRigidTiled = brush && brush.get('Rigid Tiled');

			var area0 = edge.getEnd().getFace().getArea();
			var area1 = edge.getStart().getFace().getArea();
			if (area0 && area0.getColor() == '#fff') { area0 = null; }
			if (area1 && area1.getColor() == '#fff') { area1 = null; }

			var flipAngle = ms.classifier.FLIP_ANGLE;
			if (ms.classifier.switchDirections(edge)) {
				angle = ms.util.fixAngle(angle + flipAngle + Math.PI) - flipAngle;
				var temp = startVertex;
				startVertex = endVertex;
				endVertex = temp;

				temp = area0;
				area0 = area1;
				area1 = temp;
			}
			vertexDataMap[i + ',' + startVertex.id].edges.push(i + ',' + edge.id);
			vertexDataMap[i + ',' + endVertex.id].edges.push(i + ',' + edge.id);

			// Check if edge datum already exists, if not add it.
			var newEdgeDatum = {angle: angle, brush: brush, shapeEdge: edge, edgeType: null, startVertex: startVertex, endVertex: endVertex, leftArea: area0, rightArea: area1};
			var matchingEdgeDatum = ms.classifier.findMatchingEdge(newEdgeDatum, uniqueEdgeData, isRigid);
			if (matchingEdgeDatum) {
				newEdgeDatum.edgeType = matchingEdgeDatum.edgeType;
			} else {
				var edgeLength = Infinity;
				var offset = null;
				if (isRigid) {
					offset = endVertex.getPosition().copy().minus(startVertex.getPosition()).scale(1 / cellWidth);
					// The y-axis is reversed in the shape view.
					offset.y *= -1;
					if (isRigidTiled) {
						edgeLength = brush.get('Tile Length');
						offset.normalize().scale(edgeLength);
					} else {
						edgeLength = startVertex.getPosition().distance(endVertex.getPosition()) / cellWidth;
					}
				}
				var options = {isRigid, isRigidTiled, brush, edgeLength, angle, offset};
				var edgeType = null;
				if (ms.globalSettings.get('Use Network')) {
					var dir = ms.vec3.unitVec(angle);
					var typeL = getFaceType(area0);
					var typeR = getFaceType(area1);
					var faceData = [
						{type: typeR, onRight: true},
						{type: typeL, onRight: false},
					],
					edgeType = new ms.edgeType3D(faceData, dir, options);
					// edgeType.is3D = false;
					ms.union(faceTypes, [typeL, typeR]);
				} else {
					edgeType = new ms.edgeType(isRigid, isRigidTiled, brush, edgeLength, angle, offset);
					edgeType.setAreas(area0, area1);
				}
				newEdgeDatum.edgeType = edgeType;
				uniqueEdgeData.push(newEdgeDatum);
				edgeDatum = newEdgeDatum;
			}
			edgeDataMap[i + ',' + edge.id] = newEdgeDatum;
		});
	};
	for (var i = 0; i < shapes.length; i++) {
		var s = shapes[i];
		s.getEdges().forEach(function(edge) {
			if (!edgeDataMap.hasOwnProperty(i +',' + edge.id)) {
				return;
			}			
			var startVertex = edge.getStart().getVertex().getGroup();
			var endVertex = edge.getEnd().getVertex().getGroup();

			var edgeType;
			var edgeData = edgeDataMap[i + ',' + edge.id];
			var guideStartVertex = vertexDataMap[i + ',' + edgeData.startVertex.id].vertexType;
			var guideEndVertex = vertexDataMap[i + ',' + edgeData.endVertex.id].vertexType;
			var angle = edgeData.edgeType.getAngle();
			var edgeType = edgeData.edgeType;
			edgeType.addStartVertex(guideStartVertex, angle);
			edgeType.addEndVertex  (guideEndVertex, angle);
		});
	};
	
	uniqueVertexData = ms.classifier.removeRedundantVertexTypes(uniqueVertexData);

	this.edges = [];
	for (var i = 0; i < uniqueEdgeData.length; i++) {
		this.edges.push(uniqueEdgeData[i].edgeType);
	}
	ms.classifier.snapEdgeAngles(this.edges);

	var justVertices = uniqueVertexData.map(function(datum) {
		return datum.vertexType;
	});

	if (ms.globalSettings.get('Use Network')) {
		justVertices.forEach((v) => { v.computeFaceIds(); });
		var types = {
			vertexTypes: justVertices,
			edgeTypes: this.edges,
			faceTypes
		};
		return ms.networkFactory.typesToNetworks(types);
	} else {
		return ms.familyTree.typesToGraphs(justVertices, this.edges);
	}
};

ms.classifier.prototype.setShapes = function(shapes) {
	this.is3D = shapes[0].is3D;
	var primitives;
	if (this.is3D) {
		var fileName = shapes[0].getName();
		var decorationText = ms.obj[fileName].xml;
		var decoration = ms.util.xmlTextToJson(decorationText);
		var objFile = new OBJFile(ms.obj[fileName].obj, fileName);
		objFile.parse();
		primitives = ms.graphFinder3D.find(objFile.result.models[0], decoration.decoration);
		this.xml = primitives.xml;
		this.xml.decorationText = decorationText;
	} else {
		primitives = ms.classifier.extractPrimitives2D(shapes);
		primitives.splicedEdgeTypes = {};
		// Similar to ms.shape3d.
		primitives.getSpliceEdgeType = function(faceType, dir) {
			return ms.shape3D.getSpliceEdgeType(primitives.splicedEdgeTypes, primitives.edgeTypes, faceType, dir);
		};
	}
	this.resetHierarchy();
	this.hierarchy.generate(primitives);
	this.boundaryGroups = this.hierarchy.boundaryGroups;
	
	this.fullyConnected = primitives.edgeTypes.some(function(edgeType) {
		return edgeType.isConnected()
	});
};

ms.classifier.removeRedundantVertexTypes = function(vertexData) {
	var identifiers = [];
	var uniqueData = [];
	for (var i = 0; i < vertexData.length; i++) {
		var identifierA = ms.classifier.vertexIdentifier(vertexData[i].vertexType);
		var existingIndex = identifiers.findIndex(function(identifierB) {
			if (identifierA.length != identifierB.length) {
				return false;
			}
			for (var j = 0; j < identifierA.length; j++) {
				if (identifierA[j] != identifierB[j]) {
					return false;
				}
			}
			return true;
		});
		if (existingIndex >= 0) {
			var decorationA = vertexData[i].vertexType.getDecoration();
			if (decorationA && !decorationA.isEmpty()) {
				var decorationB = uniqueData[existingIndex].vertexType.getDecoration();
				if (decorationB && !decorationB.isEmpty() && decorationA != decorationB) {
					ms.alert('Two different decorations applied to the same vertex.');
				}
				uniqueData[existingIndex] = vertexData[i]
			}
		} else {
			identifiers.push(identifierA);
			uniqueData.push(vertexData[i]);
		}
	}
	return uniqueData;
};

ms.classifier.vertexIdentifier = function(vertexType) {
	return vertexType.getConnections().map(function(connection) {
		return connection.edge.id + ',' + (connection.isAtStart ? 0 : 1)
	}).sort();
};

ms.classifier.prototype.getFamilyTree = function() {
	return this.hierarchy;
};

ms.classifier.edgeAngle = function(startVertex, endVertex) {
	var startPos = startVertex.getPosition().copy();
	var endPos = endVertex.getPosition().copy();
	// Flip y-coordinates.
	startPos.y *= -1;
	endPos.y *= -1;
	return ms.vec2.angle(startPos, endPos);
};

ms.classifier.prototype.getBoundaryGroups = function() {
	return this.boundaryGroups;
};

ms.classifier.prototype.isFullyConnected = function() {
	return this.fullyConnected;
};
