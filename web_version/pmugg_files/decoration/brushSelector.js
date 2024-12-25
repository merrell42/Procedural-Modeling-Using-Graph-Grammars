ms.brushSelector = function(container, decorationModel) {
	this.decorationModel = decorationModel;
	this.brushElements = [];
	this.brushContainer = document.createElement('span');
	this.controller = null;
	container.appendChild(this.brushContainer);

	decorationModel.addObserver(this);
	this.notify();

	var addButton = document.createElement('span');
	addButton.innerHTML ='+';
	addButton.className = 'brush-selector-button';
	var minusButton = document.createElement('span');
	minusButton.innerHTML ='-';
	minusButton.className = 'brush-selector-button';
	container.appendChild(minusButton);
	container.appendChild(addButton);
	addButton.addEventListener("click", this.onAddClick.bind(this));
	minusButton.addEventListener("click", this.onRemoveClick.bind(this));
};

ms.brushSelector.prototype.setController = function(controller) {
	this.controller = controller;
};

ms.brushSelector.prototype.notify = function() {
	var collection = this.decorationModel.getActiveCollection();
	if (!collection) {
		return;
	}
	var brushes = collection.getBrushes();	
	var activeCollection = this.decorationModel.getActiveCollection();
	while (this.brushElements.length > 0) {
		var removee = this.brushElements.pop();
		removee.remove();
	}
	for (var i = 0; i < brushes.length; i++) {
		this.addBrush(brushes[i], i);
	}
	var activeBrush = this.decorationModel.getActiveBrush();
	for (var i = 0; i < this.brushElements.length; i++) {
		this.brushElements[i].className = (brushes[i] == activeBrush) ? 'active-brush' : 'inactive-brush';
	}
};

ms.brushSelector.prototype.onAddClick = function() {
	var collection = this.decorationModel.getActiveCollection();
	collection.addBrush(this.decorationModel.notify.bind(this.decorationModel));
	var brushes = collection.getBrushes();
	this.addBrush(brushes[brushes.length - 1], brushes.length - 1);
};

ms.brushSelector.prototype.onRemoveClick = function() {
	var collection = this.decorationModel.getActiveCollection();
	collection.removeBrush();
	this.brushContainer.children[this.brushContainer.children.length-1].remove()
};

ms.brushSelector.prototype.addBrush = function(brush, i) {	
	var clickHandler = this.selectBrush.bind(this, i);
	var newChild = document.createElement('span');
	this.brushContainer.appendChild(newChild);
	this.brushElements.push(newChild);
	newChild.addEventListener("click", clickHandler, false);
	newChild.className = 'inactive-brush';

	var tiles = brush.get('Image');
	if (tiles == 'None') { tiles = ''; }
	if (tiles) {
		var tile = ms.decorationView.pickImage(tiles, 0);
		tile = tile.replace('img-', '');
		newChild.style.backgroundImage = 'url(model_synthesis_files/images/' + tile + ')';
		newChild.style.backgroundSize = 'contain';
	} else {
		newChild.style.backgroundColor = brush.getColor();
		newChild.style.borderColor = brush.getBorderColor();
	}
	brush.element = newChild;
};

ms.brushSelector.prototype.selectBrush = function(index) {
	this.decorationModel.setVertex(null);
	this.decorationModel.setBrushIndex(index);
	this.decorationModel.notify();
	if (this.decorationModel.getMode() != ms.decorationModel.Mode.AREA) {
		this.controller.updateMode(ms.objectController.modeTypes.CREATE_POLYGON);
	}
};
