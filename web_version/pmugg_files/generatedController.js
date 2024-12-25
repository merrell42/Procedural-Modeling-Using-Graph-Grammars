// The controller for the generated shapes. Handles keyboard and mouse events.
ms.generatedController = function(view, mainController) {
	this.view = view;

	this.mainController = mainController;
	this.viewport = null;
	this.cameraController = new ms.cameraController(view, this.notify.bind(this));
	this.synthesizer = new ms.synthesizer(new ms.model([0, 0, 0]), view, this.cameraController);
};

// The size of the margin of the body of the page in pixels.
ms.generatedController.BODY_MARGIN = 8;

ms.generatedController.MODES = {
	FULL: 0,
	GET_GRAMMAR: 1,
};

ms.generatedController.actionButtons = {
    SYNTHESIZE: 0,
    RESET: 1,
    PAUSE: 2,
    RESUME: 3,
    PLUS1: 4,
    PLUS10: 5,
    PLUS100: 6,
    TOGGLE: 7,
    GET_EXAMPLE: 8,
    GET_SETTINGS: 9,
    EXPORT: 10,
};

ms.generatedController.prototype.notify = function() {
	this.viewport = this.view.getViewport();
	return this.synthesizer.notify(true);
};

ms.generatedController.prototype.activate = function(opt_mode, opt_callback) {
	this.view.setMode(ms.view.modes.GRID);
	if (this.mainController.getExample().isEmpty()) {
		this.mainController.addTestObject();
	}
	
	var actionButtons = ['Restart', 'Reset', 'Pause', 'Resume', '+1', '+10', '+100'];
    var actionIcons = ['https://paulmerrell.org/wp-content/uploads/2023/07/restart.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/reset.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/pause.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/play.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/1.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/10.png', 'https://paulmerrell.org/wp-content/uploads/2023/07/100.png', ];
	if (!ms.mvp) {
		actionButtons = actionButtons.concat(['Toggle WebGL', 'Get Example', 'Get Settings', 'Export']);
	}
	this.mainController.setModeButtons([]);
	this.mainController.setActionButtons(actionButtons, actionIcons);
    this.setActionEnabled(false, false);
	this.view.getSubView(ms.view.modes.GRID).setExtents(ms.globalSettings.get('Extents'));
	this.viewport = this.view.getViewport();
	this.cameraController.activate();
	this.synthesize(false, opt_callback, opt_mode);
};

ms.generatedController.prototype.synthesize = function(keepPaused, opt_callback, opt_mode) {
	ms.counter.reset();
	this.view.setMode(ms.view.modes.GRID);
	this.view.getSubView(ms.view.modes.GRID).setExtents(ms.globalSettings.get('Extents'));
	ms.timerG.start('Create Grammar');
	this.synthesizer.setExample(this.mainController.getExample());
	ms.timerG.stop('Create Grammar');
	var self = this;
    var callback = function(report) {
        opt_callback && opt_callback(report);
        self.setActionEnabled(true, true);
    };
	if (opt_mode == ms.generatedController.MODES.GET_GRAMMAR) {
		callback(this.synthesizer.mutator.classifier.hierarchy.export());
		return;
	}
	this.synthesizer.synthesize(ms.globalSettings.get('Extents'), keepPaused, callback);
	this.notify();
};

ms.generatedController.prototype.setActionEnabled = function(paused, finished) {
    const play = paused && !finished;
    this.mainController.setActionEnabled([true, true, !paused && !finished, play, play, play, play]);
};

ms.generatedController.prototype.applyAction = function(button) {
    var actionButtons = ms.generatedController.actionButtons;
    if (button == actionButtons.SYNTHESIZE) {
        this.synthesize(false);
        this.setActionEnabled(false, false);
    } else if (button == actionButtons.RESET) {
        this.synthesize(true);
        this.setActionEnabled(true, false);
    } else if (button == actionButtons.PAUSE) {
        this.synthesizer.pause();
        this.setActionEnabled(true, false);
    } else if (button == actionButtons.RESUME) {
        this.synthesizer.resume();
        this.setActionEnabled(false, false);
    } else if (button == actionButtons.PLUS1) {
        this.synthesizer.step(1);
    } else if (button == actionButtons.PLUS10) {
        this.synthesizer.step(10);
    } else if (button == actionButtons.PLUS100) {
        this.synthesizer.step(100);
    } else if (button == actionButtons.TOGGLE) {
        ms.globalSettings.set('Use WebGL', !ms.globalSettings.get('Use WebGL'));
        this.view.setMode(ms.view.modes.GRID);
        this.view.getSubView(ms.view.modes.GRID).setExtents(ms.globalSettings.get('Extents'));
        this.notify();
    } else if (button == actionButtons.GET_EXAMPLE) {
        var xml = this.synthesizer.mutator.classifier.xml;
        navigator.clipboard.writeText(ms.exporter.exportExample(xml));
        // navigator.clipboard.writeText(ms.shapeMaker.export() + ',\'' + ms.globalSettings.export() + '\'');		
    } else if (button == actionButtons.GET_SETTINGS) {
        navigator.clipboard.writeText(ms.shapeMaker.export() + ',\'' + ms.globalSettings.export() + '\'');
    } else if (button == actionButtons.EXPORT) {
        this.export();
    }
};

ms.generatedController.prototype.export = function () {
	navigator.clipboard.writeText(this.synthesizer.exportOutput());
};

ms.generatedController.prototype.onmousemove = function (event) {
	this.cameraController.onmousemove(event);
};

ms.generatedController.prototype.onmouseup = function (event) {
	this.cameraController.onmouseup(event);
};

ms.generatedController.prototype.onmousedown = function (event) {
	this.cameraController.onmousedown(event);
	
	/* var margin = ms.generatedController.BODY_MARGIN;
	var x = event.x - margin;
	var y = event.y - margin;
	this.synthesizer.onmousedown(x, y); */
};


ms.generatedController.prototype.onmouseout = function (event) {};

ms.generatedController.prototype.onDoubleClick = function (event) {};

ms.generatedController.prototype.onRightClick = function (event) {};

ms.generatedController.prototype.onEscape = function (event) {
	this.synthesizer.pause();
};

ms.generatedController.prototype.onKeyPress = function (event) {
	var keycode = event.keyCode;
	if (49 <= keycode && keycode <= 57) {
		this.synthesizer.step(Math.pow(10, keycode - 49));
	}
	switch (keycode) {
		case 32:	this.synthesizer.resume();	break;  // Space Bar
		case 27:	this.onEscape();	        break;  // Escape
		case 69:	this.export();	    		break;  // E
		case 82:	this.synthesize(true);	    break;  // R
		case 83:	this.synthesize(false);	    break;  // S
		default:	break;
	}
};

ms.generatedController.prototype.onKeyUp = function (event) {};
