ms.connectorGroup = function() {
	this.network = null;
	// We actually use the vertexNets attached to the primalVertex which
	// usually represents the connector. It is easier to connect the vertexNets
	// when copying the network.
	this.connectors = [];

	this.id = ms.counter.add('connectorGroup');
};

ms.counter.register('connectorGroup');

ms.connectorGroup.prototype.getNetwork = function() {
	return this.network;
};

ms.connectorGroup.prototype.connectNet = function(network) {
	this.network = network;
	network.addConnectorGroup(this);
	return this;
};

ms.connectorGroup.prototype.addConnector = function(connector, index) {
	this.connectors[index] = connector;
	connector.setGroup(this);
};

ms.connectorGroup.prototype.getConnectors = function() {
	return this.connectors;
};

ms.connectorGroup.prototype.copyConnection = function(copy) {
	var copyNet = copy.getNetwork();
	var self = this;
	this.connectors = copy.getConnectors().map((vertex) => {
		return self.network.convertVertex(copyNet, vertex);
	});
};

ms.connectorGroup.prototype.export = function(copy) {
	var self = this;
	return {
		connectors: this.getConnectors().map((vertex) => {
			return self.network.vertexIndex(vertex);
		}),
	}
};

ms.connectorGroup.prototype.import = function(json) {
	var self = this;
	this.connectors = json.connectors.map((vertex) => {
		return self.network.getVertices()[vertex];
	});
};
