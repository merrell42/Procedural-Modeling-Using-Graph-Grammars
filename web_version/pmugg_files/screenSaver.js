ms.screenSaver = function(gridCanvas, offscreenCanvas) {
	this.gridCanvas = gridCanvas;
	this.offscreenCanvas = offscreenCanvas;
	this.rows = 1;
	this.columns = 1;
	this.imageCount = 0;
	this.setCount = 0;
	this.downloadLink = document.getElementById('download-link');
	this.imageStart = '';
	this.imageEnd = '';
	this.reports = [];
};

ms.screenSaver.prototype.reset = function(rows, columns, imageName) {
	this.rows = rows;
	this.columns = columns;
	this.imageCount = 0;
	this.setCount = 0;
}

ms.screenSaver.prototype.setImageName = function(imageStart, imageEnd) {
	this.imageStart = imageStart;
	this.imageEnd = imageEnd;
}

ms.screenSaver.prototype.logReport = function(report) {
	this.reports.push(report);
};

ms.screenSaver.prototype.save = function() {
	var w = this.gridCanvas.width;
	var h = this.gridCanvas.height;
	var wOff = w / this.rows;
	var hOff = h / this.columns;
	var x = this.imageCount % this.rows;
	var y = Math.floor(this.imageCount / this.rows);
	var offscreenContext = this.offscreenCanvas.getContext('2d');
	offscreenContext.drawImage(this.gridCanvas, 0, 0, w, h, x * wOff, y * hOff, wOff, hOff);
	this.imageCount++;
	if (this.imageCount >= this.rows * this.columns) {
		this.flush();
		this.setCount++;
	}
}

ms.screenSaver.getDate = function() {
	var today = new Date();
	var dd = today.getDate();
	var mm = today.getMonth() + 1; //January is 0!
	var yyyy = today.getFullYear();
	if(dd < 10) {
			dd = '0' + dd
	}
	if (mm < 10) {
		mm = '0' + mm
	} 
	return mm + '/' + dd + '/' + yyyy;
}

ms.screenSaver.prototype.flush = function() {
	if (this.imageCount == 0) {
		return;
	}
	var fileName = this.imageStart + String.fromCharCode(65 + this.setCount) + ' ' + this.imageEnd;
	this.downloadLink.setAttribute('download', fileName + '.png');
  this.downloadLink.setAttribute('href', this.offscreenCanvas.toDataURL("image/png").replace("image/png", "image/octet-stream"));
  this.downloadLink.click();
	
	var textData = fileName + '\n';
	for (var i = 0; i < this.reports.length; i++) {
		var report = this.reports[i];
		textData += report.name + ' ' + report.summary + '\n';
	}
	for (var i = 0; i < this.reports.length; i++) {
		textData += '\n';
		var report = this.reports[i];
		textData += report.name + '\n';
		textData += report.details + '\n';
	}
	textData += '\n' + ms.screenSaver.getDate();
	textData += '\n' + ms.globalSettings.report();
  var textFile = new Blob([textData], {type: 'text/plain'});

	this.downloadLink.setAttribute('download', fileName + '.txt');
  this.downloadLink.setAttribute('href', URL.createObjectURL(textFile));
  this.downloadLink.click();

	this.imageCount = 0;
	this.reports = [];
	
	var context = this.offscreenCanvas.getContext('2d');
	var w = this.gridCanvas.width;
	var h = this.gridCanvas.height;
	context.clearRect(0, 0, w, h);
}
