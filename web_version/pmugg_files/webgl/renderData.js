ms.renderData = function() {
	this.data = {};
};

ms.renderData.prototype.getTextureData = function(texture) {
	var result = this.data[texture];
	if (!result) {
		var result = {
			color: '#000000',
			texture: texture,
			positions: [],
			texcoords: [],
		}
		this.data[texture] = result;
	}
	return result;
};

ms.renderData.prototype.getColorData = function(color) {
	var result = this.data[color];
	if (!result) {
		var result = {
			color: color,
			texture: '',
			positions: [],
			texcoords: [],
		}
		this.data[color] = result;
	}
	return result;
};