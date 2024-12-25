ms.ringTransition = function(groups) {
	this.groups = [];
	for (var i = 0; i < groups.length; i++) {
		this.addGroup(groups[i]);
	}
	this.id = ms.ringTransition.count++;
};

ms.ringTransition.count = 0;

ms.ringTransition.prototype.findRings = function(rings) {
	this.groups.forEach((g) => g.findRings(rings));
};

ms.ringTransition.prototype.export = function(types) {
	return {
		groups: this.groups.map((g) => g.export(types)),
	};
};

ms.ringTransition.import = function(json, types) {
	var result = new ms.ringTransition([]);
	result.groups = json.groups.map((g) => ms.ringGroup.import(g, types));
	return result;
};

ms.ringTransition.prototype.getGroups = function() {
	return this.groups;
};

ms.ringTransition.prototype.getExtensions = function() {
	return this.extensions;
};

ms.ringTransition.prototype.addGroup = function(group) {
	this.groups.push(group);
	var rings = group.getRings();
	for (var i = 0; i < rings.length; i++) {
		rings[i].addTransitionSpot({
			transition: this,
			groupIndex: this.groups.length - 1,
			ringIndex: i,
		});
	}
};

ms.ringTransition.prototype.removeGroups = function(groups) {
	var indices = [];
	for (var i = 0; i < this.groups.length; i++) {
		indices.push(i);
	}
	var self = this;
	var removeIndices = groups.map(function(group) {
		return self.groups.indexOf(group);
	}).sort();
	for (var i = 0; i < groups.length; i++) {
		var index = removeIndices[i];
		indices[index] = -1;
		for (var j = index + 1; j < indices.length; j++) {
			indices[j]--;
		}
		groups[i].removeTransition(this);
	}
	this.groups = this.groups.filter(function(group) {
		return !groups.includes(group);
	});
	for (var i = 0; i < this.groups.length; i++) {
		var rings = this.groups[i].getRings();
		rings.forEach(function(ring) {
			ring.getTransitionSpots().forEach(function(spot) {
				if (spot.transition == self) {
					spot.groupIndex = indices[spot.groupIndex];
				}
			});
		});
	}
};

ms.ringTransition.prototype.matchGroup = function(groupA) {
	var ringsA = groupA.getRings();
	for (var i = 0; i < this.groups.length; i++) {
		var ringsI = this.groups[i].getRings();
		if (ringsA.length == ringsI.length) {
			var match = true;
			for (var j = 0; j < ringsA.length; j++) {
				if (ringsA[j] != ringsI[j]) {
					match = false;
					continue;
				}
			}
			if (match) {
				var dataI = this.groups[i].endpointData;
				var dataA = groupA.endpointData;
				var endpointOrder = dataA.map(function(datumA) {
					return dataI.findIndex(function(datumI) {
						return datumI.id == datumA.id &&
						       datumI.ringIndex == datumA.ringIndex &&
						       datumI.lineIndex == datumA.lineIndex;
					});
				});
				return endpointOrder;
			}
		}
	}
	return null;
};

ms.ringTransition.prototype.matchSet = function(setA) {
	var ringsA = setA.getRings();
	for (var i = 0; i < this.groups.length; i++) {
		var setI = this.groups[i];
		var ringsI = setI.getRings();
		if (ringsA.length == ringsI.length &&
			setA.getOuterFaceId() == setI.getOuterFaceId()) {
			var match = true;
			for (var j = 0; j < ringsA.length; j++) {
				if (ringsA[j] != ringsI[j]) {
					match = false;
					break;
				}
			}
			if (match) {
				var connectorsI = setI.getConnectors();
				var connectorsA = setA.getConnectors();
				var endpointOrder = connectorsA.map(function(connectorA) {
					return connectorsI.findIndex(function(connectorI) {
						return connectorI.equalSpots(connectorA);
					});
				});
				return endpointOrder;
			}
		}
	}
	return null;
};

ms.ringTransition.prototype.isDestructible = function() {
	for (var i = 0; i < this.groups.length; i++) {
		var group = this.groups[i];
		for (var j = 0; j < group.rings.length; j++) {
			if (group.rings[j].isEmpty()) {
				return true;
			}
		}
	}
	return false;
};