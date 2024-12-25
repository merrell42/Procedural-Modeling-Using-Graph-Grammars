ms.decorationView = function(container, decorationModel) {
	this.container = container;
	this.elements = [];
	this.decorationModel = decorationModel;
	this.decorationModel.addObserver(this);

	var brush = new ms.brush('', '', () => {});
	var area = new ms.area('', '', () => {});
	var vertexDecoration = new ms.vertexDecoration(() => {});
	this.properties = [];
	this.properties[0] = brush.copyProperties();
	this.properties[1] = area.copyProperties();
	this.properties[2] = vertexDecoration.copyProperties();

	if (!ms.mvp) {
		this.fileInput = document.createElement('input');
		this.fileInput.type='file';
		this.fileInput.multiple = true;
		this.fileInput.onchange = this.onFileChanged.bind(this);
		container.appendChild(this.fileInput);
	}

	for (var i = 0; i < 3; i++) {
		var newElement = document.createElement('div');
		container.appendChild(newElement);
		this.elements.push(newElement);		
		for (var key in this.properties[i]) {
			this.properties[i][key].draw(newElement);
		}
	}
	this.notify();
};

ms.decorationView.prototype.notify = function() {
	var mode = this.decorationModel.getMode();
	var activeObj = this.decorationModel.getActiveObject();
	for (var i = 0; i < 3; i++) {
		this.elements[i].style.display = ((i == mode) && !ms.mvp) ? '' : 'none';
	}
	if (!activeObj) {
		return;
	}
	for (var key in this.properties[mode]) {
		this.properties[mode][key].deattach();
		this.properties[mode][key].setValue(activeObj.get(key));
		this.properties[mode][key].attach(activeObj.getWholeProperty(key));
	}
};

ms.decorationView.prototype.onFileChanged = function(event) {
	var activeObj = this.decorationModel.getActiveObject();
	if (!activeObj) {
		return;
	}
	var files = event.target.files;
	var fileNames = [];
	for (var i = 0; i < files.length; i++) {
		fileNames.push(files[i].name);
	}
	activeObj.set('Image', fileNames.join(','));
	if (this.decorationModel.getMode() == ms.decorationModel.Mode.VERTEX) {
		var onLoad = function(img) {
			activeObj.set('Height', activeObj.get('Width') * img.height / img.width);
		};
		ms.decorationView.loadImage(fileNames[0], onLoad);
	}
	// To allow us to upload the same thing twice.
	this.fileInput.value = null;
};

ms.decorationView.loadImage = function(fileName, onLoad) {
	var img = new Image();
	img.addEventListener('load', function() { onLoad(img); });
	var url = './model_synthesis_files/images/' + fileName;
	ms.viewWebGL.requestCORSIfNotSameOrigin(img, url);
	img.src = url;
};

ms.decorationView.pickImage = function(fileNames, imageSeed) {
	var splitNames = fileNames.split(',');
	return splitNames[(imageSeed || 0) % splitNames.length];
};

ms.decorationView.getImage = function(fileNames, imageSeed) {
	var fileName = ms.decorationView.pickImage(fileNames, imageSeed);
	fileName = fileName.replaceAll(' ', '-');
	fileName = fileName.replaceAll('(', '');
	fileName = fileName.replaceAll(')', '');
	var img = document.getElementById(fileName);
	if (img) {
		return img;
	}
	var container = document.getElementById('images');
	var img = document.createElement('img');
	img.id = fileName;
	img.src = 'https://paulmerrell.org/wp-content/uploads/2023/06/' + fileName;
	img.style = 'display: none';
	container.appendChild(img);
	return img;
};
