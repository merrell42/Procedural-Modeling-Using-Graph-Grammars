ms.netTransition = function(networks) {
	this.networks = networks;
	this.ground = false;

	this.id = ms.counter.add('netTransition');
};

ms.counter.register('netTransition');

ms.netTransition.prototype.getNetworks = function() {
	return this.networks;
};

ms.netTransition.prototype.addNetwork = function(network) {
	this.networks.push(network);
};

ms.netTransition.prototype.export = function() {
	return {
		n:  this.networks.map ((network) => (network.export())),
		ground: this.ground,
	};
};

ms.netTransition.import = function(json, types) {
	var result = new ms.netTransition(json.n.map((network) => ms.boundNet.import(network, types)));
	result.ground = json.ground;
	return result;
};

ms.netTransition.prototype.highlight = function(view, opt_options) {
	var h = view.viewport.height;
	var size = ms.graph.HIGHLIGHTED_SIZE;
	var options0 = opt_options || {};
	var options1 = opt_options || {};
	var offset = options0.offset || new ms.vec2(20, view.canvas.height - size - 20);
	options0.rect = [offset.x, offset.x + size, offset.y, offset.y + size, 0, 100];
	options1.rect = [offset.x + size, offset.x + 2 * size, offset.y, offset.y + size, 0, 100];
	this.networks[0].draw(view, options0);
	this.networks[1].draw(view, options1);
};

ms.netTransition.prototype.print = function() {
	ms.highlight(this);
};
