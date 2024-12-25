ms.counter = {};
ms.counter.props = {};

ms.counter.register = function(name) {
	ms.counter.props[name] = 0;
};

ms.counter.add = function(name) {
	return ms.counter.props[name]++;
}

ms.counter.reset = function() {
	Object.keys(ms.counter.props).forEach((key) => {
		ms.counter.props[key] = 0;
	});
};