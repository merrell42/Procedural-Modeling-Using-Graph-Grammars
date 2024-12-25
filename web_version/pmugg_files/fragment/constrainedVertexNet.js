ms.constrainedVertexNet = function(vertex, settings) {
	this.vertex = vertex;
	this.row = -1;
	this.rowIndices = null;
	this.settings = settings;
	this.dims = settings.dims;
	// Fixed B is the position if the vertex is fixed.
	this.fixedB = null;
	this.constraints = [];
	// Extra constraints that are linearly dependent on the main constraints.
	this.linearConstraints = [];

	// The parent is one representative from a group of vertices that are constrained
	// together. They all move together in a rigid transformation.
	this.parent = null;
	this.children = [];
	// The offset is from the parent to this vertex.
	this.parentOffset = null;
};

ms.constrainedVertexNet.prototype.addChild = function(child) {
	this.children.push(child);
};

ms.constrainedVertexNet.prototype.getParent = function() {
	return this.parent;
};

ms.constrainedVertexNet.prototype.getParentOffset = function() {
	return this.parentOffset;
};

ms.constrainedVertexNet.prototype.setParent = function(parent, parentOffset) {
	// This can happen in loops.
	if (this == parent) {
		return;
	}
	this.parent = parent;
	this.parentOffset = parentOffset;
	parent.addChild(this);
	var children = this.children.slice();
	children.forEach(function(child) {
		child.setParent(parent, parentOffset.copy().add(child.getParentOffset()));
	});
	this.children = [];
};

ms.constrainedVertexNet.prototype.print = function() {
	this.vertex.print();
};

ms.constrainedVertexNet.prototype.addLinearConstraint = function(constraint) {
	this.linearConstraints.push(constraint);
	// I thought this might be needed to updateEdgeLengths in a post processing step.
	// But I don't think this is necessary.
	// this.settings.linearConstraints.push({constraint: constraint, vertex: this});
};

ms.constrainedVertexNet.parallelDet = 1e-6;
	
ms.constrainedVertexNet.prototype.origin = function() {
	return ms.transistor.origin(this.dims);
};
	
ms.constrainedVertexNet.prototype.getM = function() {
	var M = this.settings.M;
	var dims = this.dims;
	var width = M ? M.size()[1] : 0;
	if (width == 0 || this.row < 0) {
		return mathG.zeros(dims, width);
	}
	return mathG.subset(M, mathG.index(this.rowIndices, mathG.range(0, width)));
}

ms.constrainedVertexNet.prototype.getB = function() {
	if (this.fixedB) {
		return this.fixedB;
	}
	var b = this.settings.b;
	return mathG.subset(b, mathG.index(this.rowIndices, [0]));
}

ms.constrainedVertexNet.prototype.setRowM = function(rowM) {
	var M = this.settings.M;
	this.settings.M = mathG.subset(M, mathG.index(this.rowIndices, mathG.range(0, M.size()[1])), rowM);
	this.children.forEach(function(child) {
		child.setRowM(rowM);
	});
}

ms.constrainedVertexNet.prototype.setRowB = function(rowB) {
	this.settings.b = mathG.subset(this.settings.b, mathG.index(this.rowIndices, [0]), rowB);
	this.children.forEach(function(child) {
		var p = child.getParentOffset();
		child.setRowB(mathG.add(rowB, p.toMatrix()));
	});
}

ms.constrainedVertexNet.prototype.setRow = function(row) {
	this.row = row;
	this.rowIndices = this.dims == 2 ? 
		[row, row + 1] :
		[row, row + 1, row + 2];
}

// Propagate constraints coming from this vertices after it
// has been set.
ms.constrainedVertexNet.prototype.propagate = function() {
	var endpoints = this.vertex.getEndpoints().slice();
	var isBackwards = new Array(endpoints.length).fill(false);
	/* if (endpoints.length == 1) {
		endpoints.push(endpoints[0].prev());
		isBackwards.push(true);
	} */
	
	for (var i = 0; i < endpoints.length; i++) {
		var success = this.propagateEndpoint(endpoints[i], isBackwards[i]);
		if (!success) { return false; }
	}
	this.children.forEach(function(child) {
		child.propagate();
	});
	return true;
};

ms.constrainedVertexNet.hasEndpoint = function(constraints, endpoint) {
	return constraints.some(function(constraint) {
		return constraint && constraint.endpoint == endpoint;
	});
};

ms.constrainedVertexNet.prototype.propagateEndpoint = function(endpoint, isBackward) {
	// For rigid vertices, they are constrained using the parent and children.
	if (!endpoint.getEdgeType().extendable()) {
		return true;
	}
	var twin = endpoint.getTwin();
	var constraints = this.parent ? this.parent.constraints : this.constraints;
	if (ms.constrainedVertexNet.hasEndpoint(constraints, twin)) {
		return true;
	}
	var nextVertex = isBackward ? endpoint.getVertex() : endpoint.next().getVertex();
	var cVertex = this.settings.getConstrained(nextVertex);
	if (!cVertex) {
		// This is outside the scope of the current transition.
		return true;
	}
	var constraint = {endpoint, isBackward, offset: this.origin()};
	return cVertex.addConstraint(constraint);
};

ms.constrainedVertexNet.prototype.fixVertex = function(b) {
	if (this.parent) {
		var bOffset = mathG.subtract(b, this.parentOffset.toMatrix());
		return this.parent.fixVertex(bOffset);
	}
	if (this.fixedB) {
		// Do not fix vertex twice. Reject if different location. Ignore if the same.
		var bDiff = mathG.subtract(this.fixedB, b);
		var dx = bDiff.get([0, 0]);
		var dy = bDiff.get([1, 0]);
		if (Math.abs(dx) > 1e-5 || Math.abs(dy) > 1e-5) {
			return false;
		} else {
			return true;
		}
	}
	
	this.fixVertexBase(b);
	this.children.forEach(function(child) {
		var p = child.getParentOffset();
		child.fixVertexBase(mathG.add(b, p.toMatrix()));
	});
	if (!this.satisfiesConstraints(this.constraints)) {
		return false;
	}
	if (this.constraints.length == 1) {
		ms.remove(this, this.settings.singleConstraintList);
	}
	while (this.constraints.length < 2) {
		this.constraints.push(null);
	}
	return this.propagate();
};

ms.constrainedVertexNet.prototype.fixVertexBase = function(b) {
	this.row = -1;
	this.rowIndices = null;
	this.fixedB = b;
};

ms.constrainedVertexNet.prototype.fixVertexSoft = function(b) {
	if (this.parent) {
		this.constraints = [null, null];
		var bOffset = mathG.subtract(b, this.parentOffset.toMatrix());
		return this.parent.fixVertexSoft(bOffset);
	}
	this.setRowB(b);
	for (var i = 0; i < this.children.length; i++) {
		this.children[i].constraints = [null, null];
	}
	return this.propagate();
};

ms.constrainedVertexNet.prototype.constraintVertex = function(constraint) {
	var vertex;
	if (constraint.isBackwards) {
		vertex = constraint.endpoint.next().getVertex();
	} else {
		vertex = constraint.endpoint.getVertex();
	}
	return this.settings.getConstrained(vertex);
};

ms.constrainedVertexNet.prototype.addConstraint = function(constraint) {
	var {endpoint, offset} = constraint;
	if (this.parent) {
		var pOffset = this.parentOffset.copy().scale(-1);
		return this.parent.addConstraint({endpoint, offset: pOffset});
	}
	if (ms.constrainedVertexNet.hasEndpoint(this.constraints, endpoint)) {
		return true;
	}
	this.constraints.push(constraint);
	var num = this.constraints.length;
	if (num == 1) {
		this.settings.singleConstraintList.push(this);
		var u0 = this.endpointDir(endpoint);
		// TODO: Update twin.
		var twin = endpoint.getTwin();
		var endpoints = this.vertex.getEndpoints();
		for (var i = 0; i < endpoints.length; i++) {
			var endpoint1 = endpoints[i];
			if (endpoint1 == twin || endpoint1 == endpoint) {
				continue;
			}
			var u1 = this.endpointDir(endpoint1);			
			var U;
			if (this.dims == 2) {
				U = mathG.matrix([
					[u0.x, -u1.x],
					[u0.y, -u1.y],
				]);
			} else {			
				var u2 = u0.cross(u1);
				U = mathG.matrix([
					[u0.x, -u1.x, u2.x],
					[u0.y, -u1.y, u2.y],
					[u0.z, -u1.z, u2.z],
				]);
			}
			if (Math.abs(mathG.det(U)) < ms.constrainedVertexNet.parallelDet) {
				// Set the position to somewhere on the line. This is needed for the linear constraint.
				// The final position can be determined later.
				var prevVertex = this.constraintVertex(constraint);
				var b0 = prevVertex.getB();
				b0 = mathG.add(b0, offset.toMatrix());
				this.setRowB(b0);
				this.setRowM(prevVertex.getM());
				
				var success = this.propagateEndpoint(endpoint1, true);
				if (!success) { return false; }
				this.addLinearConstraint({endpoint: endpoint1.getTwin(), offset: this.origin()});
			}
		}
	} else if (num == 2) {
		ms.remove(this, this.settings.singleConstraintList);
		var success = this.applyDouble();
		if (!success) { return false; }
		// Check that the new constraint didn't get thrown out.
		if (this.constraints.length == 2) {
			success = this.propagate();
			if (!success) { return false; }
		}
	} else if (num > 2 && !this.fixedB) {
		if (!this.satisfiesConstraints([{endpoint: endpoint, offset: offset}])) {
			return false;
		}
		this.updateEdgeLength({endpoint: endpoint, offset: offset});
	}
	return true;
};

// Update the M and b value for an edge. Assuming we already know where the two vertices are.
ms.constrainedVertexNet.prototype.updateEdgeLength = function(constraint) {
	// TODO: Adjust using the offset.
	var {endpoint, isBackward} = constraint;
	// There is nothing to update if the endpoint is not free.
	if (!endpoint.getEdgeType().extendable()) {
		return;
	}
	
	var M = this.settings.M;
	var b = this.settings.b;
	var vertex1 = this.constraintVertex(constraint);
	var offset = this.origin().copy();
	while (vertex1.getParent()) {
		offset.add(vertex1.parentOffset);
		vertex1 = vertex1.getParent();
	}
	if (vertex1.constraints.length >= 2) {		
		offset.add(constraint.offset);
		var m0 = this.getM();
		var b0 = this.getB();
		var m1 = vertex1.getM();
		var b1 = vertex1.getB();
		// I think this is right.
		b1 = mathG.add(b1, offset.toMatrix());
		var v = this.endpointDir(endpoint, isBackward);
		// Do not replace with toMatrix. This is its transpose.
		v = mathG.matrix([[v.x, v.y]])
		var mEdge = mathG.multiply(v, mathG.subtract(m0, m1));
		var bEdge = mathG.multiply(v, mathG.subtract(b0, b1));
		var edgeRow1 = this.edgeRow(endpoint);			
		M = mathG.subset(M, mathG.index([edgeRow1], mathG.range(0, M.size()[1])), mEdge);
		b = mathG.subset(b, mathG.index([edgeRow1], [0]), bEdge);
	} else {
		// This happens with Factory - Tower 2 when we have two parallel rigid constraints.
		// the position of vertex1 has not yet been determined.
		
		vertex1.addLinearConstraint({endpoint: constraint.endpoint.getTwin(), offset: this.origin()});
	}
};

ms.constrainedVertexNet.dot = function(a, b) {
	if (ms.globalSettings.get('Fast Matrix Math')) {
		return mathG.dot(a, b);
	} else {
		return mathG.multiply(mathG.transpose(a), b).get([0, 0]);
	}
}

ms.constrainedVertexNet.prototype.emptyM = function() {
	if (this.fixedB) {
		return true;
	}
	var M = this.getM();
	var width = M.size()[1];
	if (width == 0) {
		return true;
	}
	for (var i = 0; i < 2; i++) {
		for (var j = 0; j < width; j++) {
			if (Math.abs(M.data[i][j]) > 1e-6) {
				return false;
			}
		}		
	}
	return true;
};

ms.constrainedVertexNet.prototype.satisfiesConstraints = function(constraints) {	
	var lengthMin = ms.transistor.defaultLengthMin;
	// I think this should be deleted.
	// Not sure if I need to implement this or not. I lost context since the plagiarism thing.
	// if (constraints.length > 0 && !this.emptyM()) {
	// 	ms.alert('satisfies constraints only implemented for fixed vertices.');
	// }
	for (var i = 0; i < constraints.length; i++) {
		if (!constraints[i]) {
			continue;
		}
		var {endpoint, isBackward} = constraints[i];
		var u = this.endpointDir(endpoint, isBackward);
		var v = mathG.matrix([[-u.y], [u.x]]);
		u = u.toMatrix();
		var vertex1 = this.constraintVertex(constraints[i]);

		// Ignore vertices that already include this constraint.
		if (!vertex1.fixedB && ms.constrainedVertexNet.hasEndpoint(vertex1.constraints, endpoint.getTwin())) {
			continue;
		}
		var b0 = vertex1.getB();
		var b1 = this.getB();
		var v0 = ms.constrainedVertexNet.dot(v, mathG.add(b0, constraints[i].offset.toMatrix()))
		var v1 = ms.constrainedVertexNet.dot(v, b1);
		if (Math.abs(v0 - v1) > 1e-5) {
			return false;
		}
		
		// This checks that the edge lengths can be positive.
		/* 
		var u0 = ms.constrainedVertexNet.dot(u, b0);
		var u1 = ms.constrainedVertexNet.dot(u, b1);
		if (u1 < u0 + lengthMin) {			
			// I'm not sure the case where M is present is working, especially for checking in the v-direction.
			// if (!vertex1.emptyM()) {
			// 	ms.alert('satisfies constraints only implemented for fixed vertices.');
			// }			
			var m0 = vertex1.getM();
			var m1 = this.getM();
			// s = u * m1 + u * m0
			var s = mathG.subtract(mathG.multiply(mathG.transpose(u), m1), mathG.multiply(mathG.transpose(u), m0));
			// Check if for some value of M * x there is a positive edge length.
			var positive = false;
			for (var j = 0; j < s.data[0].length; j++) {
				if (s.data[0][j] > 1e-4) {
					positive = true;
					break;
				}
			}
			if (!positive) {
				// I don't think this is needed, but I'm unsure. It conflicts with the Factory Tower 2 example when there
				// are two parallel rigid lines.
				ms.alert('No positive edge length');
				// return false;
			}
		} */
	}
	return true;
};

ms.constrainedVertexNet.prototype.edgeRow = function(endpoint) {
	return this.settings.getEdgeRow(endpoint.getLine());
};

ms.constrainedVertexNet.prototype.constraintM = function(constraint) {
	var vertex = this.constraintVertex(constraint);
	return vertex.getM();
};

ms.constrainedVertexNet.prototype.constraintB = function(constraint) {
	var vertex = this.constraintVertex(constraint);
	return vertex.getB();
};

ms.constrainedVertexNet.prototype.endpointDir = function(endpoint, isBackward) {
	return endpoint.getDir();
	
	if (endpoint.overrideDir) {
		return endpoint.overrideDir;
	}
	var dir;
	var edge = endpoint.edge;
	var vertex0 = edge.getEndpoint(0).getVertex();
	var vertex1 = edge.getEndpoint(1).getVertex();
	if (!this.settings.freeVertices.includes(vertex0)) {
		dir = edge.getCore().getEndpoints()[0].getDir().copy();
	} else if (!this.settings.freeVertices.includes(vertex1)) {
		dir = edge.getCore().getEndpoints()[1].getTwin().getDir().copy();
	} else {
		dir = edge.getCore().getEdgeType().getDir().copy();
		if (this.settings.angle != 0) {
			dir = dir.rotate(this.settings.angle);
		}
	}
	if (isBackward ^ (endpoint.edgeIndex == 1)) {
		dir.scale(-1);
	}
	return dir;
};

ms.constrainedVertexNet.prototype.applySingle = function() {
	var constraint = this.constraints[0];
	var {endpoint, offset, isBackward} = this.constraints[0];
	var edgeRow = this.edgeRow(endpoint);
	var M = this.settings.M;
	var rows = M.size()[0];
	var cols = M.size()[1];
	var newColumn = mathG.zeros(rows, 1);
	newColumn.set([edgeRow, 0], 1);
	this.settings.edgeRows.push(edgeRow);

	var v = this.endpointDir(endpoint, isBackward);
	var dims = this.dims;
	var setRowV = function(row) {	
		newColumn.set([row,     0], v.x);
		newColumn.set([row + 1, 0], v.y);
		if (dims == 3) {
			newColumn.set([row + 2, 0], v.z);
		}
	}
	setRowV(this.row);
	this.children.forEach(function(child) {
		setRowV(child.row);
	});

	this.setRowM(this.constraintM(constraint));
	this.setRowB(mathG.add(this.constraintB(constraint), offset.toMatrix()));
	
	this.settings.M = mathG.concat(this.settings.M, newColumn);

	// Update the edge length of any linear constraints.
	var self = this;
	this.linearConstraints.forEach(function(linearConstraint) {
		self.updateEdgeLength(linearConstraint);
	});
	
	this.constraints.push(null);
	ms.remove(this, this.settings.singleConstraintList);
	return this.propagate();
};

ms.constrainedVertexNet.prototype.applyDouble = function() {
	var endpoint0 = this.constraints[0].endpoint;
	var endpoint1 = this.constraints[1].endpoint;
	var offset0 = this.constraints[0].offset;
	var offset1 = this.constraints[1].offset;
	var isBackward0 = this.constraints[0].offset;
	var isBackward1 = this.constraints[1].offset;
	var u0 = this.endpointDir(endpoint0, isBackward0);
	var u1 = this.endpointDir(endpoint1, isBackward1);
	var U;
	if (this.dims == 2) {		
		var U = mathG.matrix([
			[u0.x, -u1.x],
			[u0.y, -u1.y],
		]);
	} else {
		var u2 = u0.cross(u1);
		U = mathG.matrix([
			[u0.x, -u1.x, u2.x],
			[u0.y, -u1.y, u2.y],
			[u0.z, -u1.z, u2.z],
		]);
	}
	
	
	var edgeRow0 = this.edgeRow(endpoint0);
	var edgeRow1 = this.edgeRow(endpoint1);
	var M = this.settings.M;
	var b = this.settings.b;
	var m0 = this.constraintM(this.constraints[0]);
	var m1 = this.constraintM(this.constraints[1]);
	var b0 = mathG.add(this.constraintB(this.constraints[0]), offset0.toMatrix());
	var b1 = mathG.add(this.constraintB(this.constraints[1]), offset1.toMatrix());
	var bDiff = mathG.subtract(b1, b0);
	if (Math.abs(mathG.det(U)) < ms.constrainedVertexNet.parallelDet) {
		// The next part is untested. It used to be:
		// var v = mathG.matrix([[-u0.y], [u0.x]]);
		// if (math.abs(mathG.dot(bDiff, v)) > 1e-5) {
			
		var bDiffVec = new ms.vec2(bDiff.get([0, 0]), bDiff.get([1, 0]));
		bDiffVec.normalize();
		if ((Math.abs(u0.dot(u1)) < 1 - 1e-5) ||
			(Math.abs(u0.dot(bDiffVec)) < 1 - 1e-5)) {
			return false;
		}
		// If this new constraint is colinear with the first, remove it and add as a linear constraint.
		this.addLinearConstraint(this.constraints.pop());
		this.settings.singleConstraintList.push(this);
		return true;
	}
	
	var uInv = mathG.inv(U);
	var bEdge = mathG.multiply(uInv, bDiff);
	var bEdge0 = bEdge.get([0, 0]);
	var bEdge1 = bEdge.get([1, 0]);
	var hasM = (M.size()[1] > 0);
	if (hasM) {
		var mEdge = mathG.multiply(uInv, mathG.subtract(m1, m0));
		var mEdge0 = mathG.subset(mEdge, mathG.index([0], mathG.range(0, M.size()[1])));
		var mEdge1 = mathG.subset(mEdge, mathG.index([1], mathG.range(0, M.size()[1])));
		M = mathG.subset(M, mathG.index([edgeRow0], mathG.range(0, M.size()[1])), mEdge0);
		M = mathG.subset(M, mathG.index([edgeRow1], mathG.range(0, M.size()[1])), mEdge1);
	}
	b.set([edgeRow0, 0], bEdge0);
	b.set([edgeRow1, 0], bEdge1);

	this.settings.M = M;
	if (hasM) {
		var mVertex0 = mathG.add(m0, mathG.multiply(u0.toMatrix(), mEdge0));
		// var mVertex1 = mathG.add(m1, mathG.multiply(u1.toMatrix(), mEdge1));
		this.setRowM(mVertex0);
	}
	var bVertex0 = mathG.add(b0, mathG.multiply(u0.toMatrix(), bEdge0));
	// var bVertex1 = mathG.add(b1, mathG.multiply(u1.toMatrix(), bEdge1));
	this.setRowB(bVertex0);
	
	// This should give the same result.
	// this.setRowM(mVertex1);
	// this.setRowB(bVertex1);

	// Update the edge length of any linear constraints.
	var self = this;
	this.linearConstraints.forEach(function(linearConstraint) {
		self.updateEdgeLength(linearConstraint);
	});	
	return this.satisfiesConstraints(this.linearConstraints);
};

ms.constrainedVertexNet.prototype.setRigidConstraint = function(vertexB, offsetB) {
	if (this.parent) {
		offsetB.add(this.parentOffset);
		this.parent.setRigidConstraint(vertexB, offsetB);
	} else if (vertexB.getParent()) {
		offsetB.minus(vertexB.parentOffset);
		this.setRigidConstraint(vertexB.getParent(), offsetB);
	} else {
		// We have reached the top level. This vertex and vertex B have no parents.
		vertexB.setParent(this, offsetB);
	}
};
