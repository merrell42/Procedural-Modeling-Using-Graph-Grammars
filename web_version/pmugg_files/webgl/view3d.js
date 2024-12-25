ms.view3d = function(canvas) {
	this.canvas = canvas;
	this.mvMatrix = mat4.create();
	this.pMatrix = mat4.create();
	this.viewport = new ms.viewport3d();
	
	try {
		var gl = canvas.getContext('experimental-webgl');	
	} catch (e) {
	}
	if (!gl) {
		alert("Could not initialise WebGL, sorry :-(");
	}
	this.gl = gl;
	this.faceVerticesBuffer = null;
	this.faceVerticesColorBuffer = null;
	this.faceVerticesIndexBuffer = null;

	this.lineVerticesBuffer = null;
	this.lineVerticesColorBuffer = null;
	this.lineVerticesIndexBuffer = null;
	
	this.lineVertices = [];
	this.lineColors = [];
	this.lineVertexIndices = [];

	this.faceVertices = [];
	this.faceColors = [];
	this.faceVertexIndices = [];

	this.initShaders();
	gl.clearColor(0.0, 0.0, 0.0, 1.0);
	gl.enable(gl.DEPTH_TEST);
};

ms.view3d.SCALE = 0.01;
// I don't even know where the 0.2 comes from.
ms.view3d.GENERATED_SCALE = 0.2 / ms.view3d.SCALE;
// Default number of segments in the cylinder
ms.view3d.CYCLINDER_SEGS = 4;
// Default cylinder radius.
ms.view3d.CYCLINDER_RADIUS = 0.5;

ms.view3d.prototype.getViewport = function() {
	return this.viewport;
};

ms.view3d.prototype.resize = function(width, height) {
	this.canvas.width = width;
	this.canvas.height = height;
	this.gl.viewportWidth = width;
	this.gl.viewportHeight = height;
	this.viewport.resize(width, height);
};

ms.view3d.prototype.activate = function() {
	this.canvas.style.display = '';
};

ms.view3d.prototype.deactivate = function() {
	this.canvas.style.display = 'none';
};

ms.view3d.prototype.redraw = function(driver) {	
	this.lineVertices = [];
	this.lineColors = [];
	this.lineVertexIndices = [];
	this.faceVertices = [];
	this.faceColors = [];
	this.faceVertexIndices = [];

	// this.initTestBuffers();

	if (ms.highlightedElement) {
		ms.highlightedElement.highlight(this);
	}
	var renderables = driver.getRenderables();
	var offset = ms.vec2.ORIGIN;
	for (var i = 0; i < renderables.length; i++) {
		renderables[i].draw(this, offset);
	}
	this.initBuffers();
	this.drawScene();
};

ms.view3d.getShader = function(gl, id) {
	var shaderScript = document.getElementById(id);
	if (!shaderScript) {
		return null;
	}

	var str = "";
	var k = shaderScript.firstChild;
	while (k) {
		if (k.nodeType == 3) {
			str += k.textContent;
		}
		k = k.nextSibling;
	}

	var shader;
	if (shaderScript.type == "x-shader/x-fragment") {
		shader = gl.createShader(gl.FRAGMENT_SHADER);
	} else if (shaderScript.type == "x-shader/x-vertex") {
		shader = gl.createShader(gl.VERTEX_SHADER);
	} else {
		return null;
	}
	gl.shaderSource(shader, str);
	gl.compileShader(shader);

	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		alert(gl.getShaderInfoLog(shader));
		return null;
	}

	return shader;
};

ms.view3d.stringToRgb = function(hex) {
  var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  if (result) {
	  return {
		r: parseInt(result[1], 16) / 255.0,
		g: parseInt(result[2], 16) / 255.0,
		b: parseInt(result[3], 16) / 255.0
	  };
  }
  result = /^#?([a-f\d]{1})([a-f\d]{1})([a-f\d]{1})$/i.exec(hex);
  if (result) {
	  return {
		r: parseInt(result[1], 16) / 15.0,
		g: parseInt(result[2], 16) / 15.0,
		b: parseInt(result[3], 16) / 15.0
	  };
  }
  result = /rgb\((\d{1,3}), (\d{1,3}), (\d{1,3})\)/.exec(hex);
  if (result) {
	  return {
		r: parseInt(result[1], 10) / 255.0,
		g: parseInt(result[2], 10) / 255.0,
		b: parseInt(result[3], 10) / 255.0
	  };
  }
  ms.alert('Could not parse color');
  return { r: 0.5, g: 0.3, b: 0.5};
};

ms.view3d.prototype.initShaders = function() {
	var gl = this.gl;
	var fragmentShader = ms.view3d.getShader(gl, "shader-fs");
	var vertexShader = ms.view3d.getShader(gl, "shader-vs");

	this.shaderProgram = gl.createProgram();
	gl.attachShader(this.shaderProgram, vertexShader);
	gl.attachShader(this.shaderProgram, fragmentShader);
	gl.linkProgram(this.shaderProgram);

	if (!gl.getProgramParameter(this.shaderProgram, gl.LINK_STATUS)) {
			alert("Could not initialise shaders");
	}

	gl.useProgram(this.shaderProgram);

	this.shaderProgram.vertexPositionAttribute = gl.getAttribLocation(this.shaderProgram, "aVertexPosition");
	gl.enableVertexAttribArray(this.shaderProgram.vertexPositionAttribute);

	this.shaderProgram.vertexColorAttribute = gl.getAttribLocation(this.shaderProgram, "aVertexColor");
	gl.enableVertexAttribArray(this.shaderProgram.vertexColorAttribute);

	this.shaderProgram.pMatrixUniform = gl.getUniformLocation(this.shaderProgram, "uPMatrix");
	this.shaderProgram.mvMatrixUniform = gl.getUniformLocation(this.shaderProgram, "uMVMatrix");
};

ms.view3d.prototype.setMatrixUniforms = function() {
	var gl = this.gl;
	gl.uniformMatrix4fv(this.shaderProgram.pMatrixUniform, false, this.pMatrix);
	gl.uniformMatrix4fv(this.shaderProgram.mvMatrixUniform, false, this.mvMatrix);
};

ms.view3d.CUBE_VERTICES = [
	// Front face
	-1.0, -1.0,  1.0,
	 1.0, -1.0,  1.0,
	 1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,

	// Back face
	-1.0, -1.0, -1.0,
	-1.0,  1.0, -1.0,
	 1.0,  1.0, -1.0,
	 1.0, -1.0, -1.0,

	// Top face
	-1.0,  1.0, -1.0,
	-1.0,  1.0,  1.0,
	 1.0,  1.0,  1.0,
	 1.0,  1.0, -1.0,

	// Bottom face
	-1.0, -1.0, -1.0,
	 1.0, -1.0, -1.0,
	 1.0, -1.0,  1.0,
	-1.0, -1.0,  1.0,

	// Right face
	 1.0, -1.0, -1.0,
	 1.0,  1.0, -1.0,
	 1.0,  1.0,  1.0,
	 1.0, -1.0,  1.0,

	// Left face
	-1.0, -1.0, -1.0,
	-1.0, -1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0,  1.0, -1.0
];

// Ambient and diffuse coefficients.
ms.view3d.FACE_COLORS = [
	[0.3,  1.0],    // Top face: white
	[0.0,  0.5],    // Bottom face: red
	[0.1,  0.8],    // Front face: green
	[0.1,  0.8],    // Back face: blue
	[0.2,  0.7],    // Left face: yellow
	[0.2,  0.7]     // Right face: purple
];

// This array defines each face as two triangles, using the
// indices into the vertex array to specify each triangle's
// position.
ms.view3d.CUBE_VERTEX_INDICES = [
	0,  1,  2,      0,  2,  3,    // front
	4,  5,  6,      4,  6,  7,    // back
	8,  9,  10,     8,  10, 11,   // top
	12, 13, 14,     12, 14, 15,   // bottom
	16, 17, 18,     16, 18, 19,   // right
	20, 21, 22,     20, 22, 23    // left
];

ms.view3d.addCube = function(x, y, z, c, vertices, colors, vertexIndices) {
	var indexStart = vertices.length / 3;
	var cubeVertices = ms.view3d.CUBE_VERTICES;
	var cubeColors = ms.view3d.CUBE_COLORS;
	var faceColors = ms.view3d.FACE_COLORS;
	var cubeVertexIndices = ms.view3d.CUBE_VERTEX_INDICES;
	for (var i = 0; 3 * i < cubeVertices.length; i++) {
		vertices.push(x + cubeVertices[3 * i    ] / 2);
		vertices.push(y + cubeVertices[3 * i + 1] / 2);
		vertices.push(z + cubeVertices[3 * i + 2] / 2);
		var faceIndex = Math.floor(i / 4);
		for (var j = 0; j < 3; j++) {
			// colors.push(cubeColors[faceIndex][j]);
			// colors.push(c[j] / 255);
			var faceColor = faceColors[faceIndex];
			colors.push(faceColor[0] + faceColor[1] * c[j] / 255);
		}
		colors.push(1.0);
	}
	
	for (var i = 0; i < cubeVertexIndices.length; i++) {
		vertexIndices.push(indexStart + cubeVertexIndices[i]);
	}
};



ms.view3d.LINE_VERTICES = [
	-1.0, -1.0, -1.0,
	 1.0, -1.0, -1.0,
	-1.0,  1.0, -1.0,
	 1.0,  1.0, -1.0,
	-1.0, -1.0,  1.0,
	 1.0, -1.0,  1.0,
	-1.0,  1.0,  1.0,
	 1.0,  1.0,  1.0,
];

ms.view3d.LINE_VERTEX_INDICES = [
	0, 1,
	2, 3,
	4, 5,
	6, 7,

	0, 2,
	1, 3,
	4, 6,
	5, 7,

	0, 4,
	1, 5,
	2, 6,
	3, 7,
];

ms.view3d.addLines = function(x, y, z, vertices, colors, vertexIndices) {
	var indexStart = vertices.length / 3;
	var lineVertices = ms.view3d.LINE_VERTICES;
	var faceColors = ms.view3d.FACE_COLORS;
	var lineVertexIndices = ms.view3d.LINE_VERTEX_INDICES;
	for (var i = 0; 3 * i < lineVertices.length; i++) {
		vertices.push(x + lineVertices[3 * i    ] / 2);
		vertices.push(y + lineVertices[3 * i + 1] / 2);
		vertices.push(z + lineVertices[3 * i + 2] / 2);
		for (var j = 0; j < 3; j++) {
			colors.push(0);
		}
		colors.push(1.0);
	}
	
	for (var i = 0; i < lineVertexIndices.length; i++) {
		vertexIndices.push(indexStart + lineVertexIndices[i]);
	}
};

ms.view3d.prototype.drawPoint = function(position, color) {
	// ms.alert('Draw Point not implemented.');
};

ms.view3d.ARROW_MAIN_LENGTH = 10;
ms.view3d.ARROW_TIP_LENGTH = 6;
ms.view3d.ARROW_MAIN_RADIUS = 1;
ms.view3d.ARROW_TIP_RADIUS = 3;

ms.view3d.prototype.drawArrow = function(startPos, endPos, color) {
	var rgb = ms.view3d.stringToRgb(color);
	var color = [rgb.r, rgb.g, rgb.b, 1.0];
	
	var u = endPos.copy().minus(startPos).normalize();
	var endMain = startPos.copy().add(u.copy().scale(ms.view3d.ARROW_MAIN_LENGTH));
	var startTip = startPos.copy().add(u.copy().scale(ms.view3d.ARROW_MAIN_LENGTH + 0.01));
	var endTip = startPos.copy().add(u.copy().scale(ms.view3d.ARROW_MAIN_LENGTH + ms.view3d.ARROW_TIP_LENGTH));
	this.drawCylinder(startPos, endMain, color, false, ms.view3d.ARROW_MAIN_RADIUS, 6);
	this.drawCylinder(endMain, startTip, color, true, ms.view3d.ARROW_TIP_RADIUS, 6); // Cap
	this.drawCylinder(startTip, endTip, color, true, ms.view3d.ARROW_TIP_RADIUS, 6);
};

ms.view3d.prototype.drawEdge = function(startPos, endPos, color, offsetScale, lineWidth) {
	var indexStart = this.lineVertices.length / 3;
	
	var rgb = ms.view3d.stringToRgb(color);
	var color = [rgb.r, rgb.g, rgb.b, 1.0];
	this.drawCylinder(startPos, endPos, color, false, lineWidth / 2);
};

ms.view3d.QUAD_VERTICES = [
	// Front face
	-1.0, -1.0,  1.0,
	 1.0, -1.0,  1.0,
	 1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,

	// Back face
	-1.0, -1.0, -1.0,
	-1.0,  1.0, -1.0,
	 1.0,  1.0, -1.0,
	 1.0, -1.0, -1.0,

	// Top face
	-1.0,  1.0, -1.0,
	-1.0,  1.0,  1.0,
	 1.0,  1.0,  1.0,
	 1.0,  1.0, -1.0,

	// Bottom face
	-1.0, -1.0, -1.0,
	 1.0, -1.0, -1.0,
	 1.0, -1.0,  1.0,
	-1.0, -1.0,  1.0,

	// Right face
	 1.0, -1.0, -1.0,
	 1.0,  1.0, -1.0,
	 1.0,  1.0,  1.0,
	 1.0, -1.0,  1.0,

	// Left face
	-1.0, -1.0, -1.0,
	-1.0, -1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0,  1.0, -1.0
];

// This array defines each face as two triangles, using the
// indices into the vertex array to specify each triangle's
// position.
ms.view3d.QUAD_VERTEX_INDICES = [
	0, 1, 2, 1, 2, 3
];

ms.view3d.prototype.drawQuad = function(corners, color) {
	var vertices = this.faceVertices;
	
	var indexStart = this.faceVertices.length / 3;
	var quadVertexIndices = ms.view3d.QUAD_VERTEX_INDICES;
	// Colors for each of the four vertices.
	for (var k = 0; k < 4; k++) {
		for (var j = 0; j < 4; j++) {
			this.faceColors.push(color[j]);
		}
	}

	var masterScale = ms.view3d.SCALE;
	for (var i = 0; i < 4; i++) {
		var corner = corners[i]
		vertices.push(masterScale * corner.x);
		vertices.push(masterScale * corner.y);
		vertices.push(masterScale * corner.z);
	}

	for (var i = 0; i < quadVertexIndices.length; i++) {
		var index = quadVertexIndices[i];
		this.faceVertexIndices.push(indexStart + index);
	}
};

// This might not be needed if we draw the points as polygons.
// The corners are different from drawQuad. They an array of size 9.
ms.view3d.prototype.drawTriangle = function(corners, color) {
	var vertices = this.faceVertices;
	
	var indexStart = this.faceVertices.length / 3;
	// Colors for each of the three vertices.
	for (var k = 0; k < 3; k++) {
		for (var j = 0; j < 4; j++) {
			this.faceColors.push(color[j]);
		}
	}

	var masterScale = ms.view3d.SCALE;
	for (var i = 0; i < 9; i++) {
		vertices.push(masterScale * corners[i]);
	}

	for (var i = 0; i < 3; i++) {
		this.faceVertexIndices.push(indexStart + i);
	}
};

// Draws a circle around v0. r0 and r1 are the inner and outer radius.
ms.view3d.prototype.drawCircle = function(v0, r0, r1, color) {
	var numSegs = 30;

	var n = new ms.vec3(0, 0, 1);
	var {u, v} = n.orthonormal();
	for (var i = 0; i < numSegs; i++) {
		var angle0 =  i      / numSegs * 2 * Math.PI;
		var angle1 = (i + 1) / numSegs * 2 * Math.PI;
		var theta0 = ms.vec2.unitVec(angle0);
		var theta1 = ms.vec2.unitVec(angle1);
		theta00 = u.copy().scale(r0 * theta0.x).add(v.copy().scale(r0 * theta0.y));
		theta01 = u.copy().scale(r0 * theta1.x).add(v.copy().scale(r0 * theta1.y));
		theta10 = u.copy().scale(r1 * theta0.x).add(v.copy().scale(r1 * theta0.y));
		theta11 = u.copy().scale(r1 * theta1.x).add(v.copy().scale(r1 * theta1.y));
		var corners = [
			v0.copy().add(theta00),
			v0.copy().add(theta01),
			v0.copy().add(theta10),
			v0.copy().add(theta11),
		];
		this.drawQuad(corners, color);
	}
};

// Draws a cylinder or a cone around v0 and v1. Draws a cone if isCone is true. The tip of the cone is v1.
ms.view3d.prototype.drawCylinder = function(v0, v1, color, opt_isCone, opt_radius, opt_segments) {
	var r = opt_radius || ms.view3d.CYCLINDER_RADIUS;
	var numSegs = opt_segments || ms.view3d.CYCLINDER_SEGS;
	var isCone = opt_isCone || false;

	var n = v1.copy().minus(v0);
	var {u, v} = n.orthonormal();
	for (var i = 0; i < numSegs; i++) {
		var angle0 =  i      / numSegs * 2 * Math.PI;
		var angle1 = (i + 1) / numSegs * 2 * Math.PI;
		var theta0 = ms.vec2.unitVec(angle0);
		var theta1 = ms.vec2.unitVec(angle1);
		theta0 = u.copy().scale(r * theta0.x).add(v.copy().scale(r * theta0.y));
		theta1 = u.copy().scale(r * theta1.x).add(v.copy().scale(r * theta1.y));
		var corners = [
			v0.copy().add(theta0),
			v0.copy().add(theta1),
			isCone ? v1 : v1.copy().add(theta0),
			isCone ? v1 : v1.copy().add(theta1),
		];
		this.drawQuad(corners, color);
	}
};

ms.view3d.prototype.initTestBuffers = function(model) {
	var gl = this.gl;

	var vertices = this.faceVertices;
	var colors = this.faceColors;
	var vertexIndices = this.faceVertexIndices;

	var lineVertices = this.lineVertices;
	var lineColors = this.lineColors;
	var lineVertexIndices = this.lineVertexIndices;
	
	var extents = [6, 6, 1];
	for (var x = 0; x < extents[0]; x++) {
		var xp = x - extents[0] / 2 + 0.5;
		for (var y = 0; y < extents[1]; y++) {
			var yp = y - extents[1] / 2 + 0.5;
			for (var z = 0; z < extents[2]; z++) {
				var zp = z + 0.5 - 1;
				// var states = model.getCell(x, y, z).getActiveStates();
				// if (states.length > 0) {
					// var stateValue = states[0].getValue();
					// if (!!stateValue.getKey()) {
						var color = (x + y + z) % 2 ? [255, 0, 0] : [0, 255, 0];
						ms.view3d.addCube(xp, yp, zp, color, vertices, colors, vertexIndices);
						ms.view3d.addLines(xp, yp, zp, lineVertices, lineColors, lineVertexIndices);
					//}
				// }
			}
		}
	}
	
	// Add background surface.
	/* var indexStart = vertices.length / 3;
	vertices = vertices.concat([
		-extents[0] / 2 - 1, -extents[1] / 2 - 1, -0.01,
		 extents[0] / 2 + 1, -extents[1] / 2 - 1, -0.01,
		 extents[0] / 2 + 1,  extents[1] / 2 + 1, -0.01,
		-extents[0] / 2 - 1,  extents[1] / 2 + 1, -0.01,
	]);
	for (var i = 0; i < 4; i++) {
		colors = colors.concat([0.8, 0.8, 0.8, 1]);
	}
	var backgroundIndices = [0, 1, 2, 0, 2, 3];
	for (var i = 0; i < backgroundIndices.length; i++) {
		vertexIndices.push(indexStart + backgroundIndices[i]);
	} */
}

ms.view3d.prototype.initBuffers = function() {
	var gl = this.gl;
	
	// Now pass the list of vertices into WebGL to build the shape. We
	// do this by creating a Float32Array from the JavaScript array,
	// then use it to fill the current vertex buffer.
	this.faceVerticesBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, this.faceVerticesBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.faceVertices), gl.STATIC_DRAW);

	this.faceVerticesColorBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, this.faceVerticesColorBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.faceColors), gl.STATIC_DRAW);

	// Build the element array buffer; this specifies the indices
	// into the vertex array for each face's vertices.
	this.faceVerticesIndexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.faceVerticesIndexBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(this.faceVertexIndices), gl.STATIC_DRAW);
	
	this.lineVerticesBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, this.lineVerticesBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.lineVertices), gl.STATIC_DRAW);

	this.lineVerticesColorBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, this.lineVerticesColorBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(this.lineColors), gl.STATIC_DRAW);

	this.lineVerticesIndexBuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.lineVerticesIndexBuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(this.lineVertexIndices), gl.STATIC_DRAW);
};

ms.view3d.prototype.drawScene = function() {
	var gl = this.gl;
	const ext = gl.getExtension('OES_element_index_uint');
	gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
	gl.clearColor(255, 255, 255, 1.0);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

	var fov = 45;
	var aspectRatio = gl.viewportWidth / gl.viewportHeight;
	var nearClipping = 0.1;
	var farClipping = 1000;
	mat4.perspective(this.viewport.getFov(), aspectRatio, nearClipping, farClipping, this.pMatrix);

	// TODO: Do this in the constructor.
	var cameraMatrix = mat4.create();
	var viewMatrix = mat4.create();
	mat4.lookAt(this.viewport.getEye().toArray(), this.viewport.getTarget().toArray(), this.viewport.getUp().toArray(), cameraMatrix);
	mat4.multiply(this.pMatrix, cameraMatrix, this.pMatrix);

	mat4.identity(this.mvMatrix);

	mat4.translate(this.mvMatrix, [0, 0, 0]);	

	// Draw the cube by binding the array buffer to the cube's vertices
	// array, setting attributes, and pushing it to GL.
	gl.bindBuffer(gl.ARRAY_BUFFER, this.faceVerticesBuffer);
	gl.vertexAttribPointer(this.shaderProgram.vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0);

	// Set the colors attribute for the vertices.
	gl.bindBuffer(gl.ARRAY_BUFFER, this.faceVerticesColorBuffer);
	gl.vertexAttribPointer(this.shaderProgram.vertexColorAttribute, 4, gl.FLOAT, false, 0, 0);

	// Draw the faces.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.faceVerticesIndexBuffer);
	this.setMatrixUniforms();

	gl.drawElements(gl.TRIANGLES, this.faceVertexIndices.length, gl.UNSIGNED_INT, 0);	

	gl.bindBuffer(gl.ARRAY_BUFFER, this.lineVerticesBuffer);
	gl.vertexAttribPointer(this.shaderProgram.vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0);

	// Set the colors attribute for the vertices.
	gl.bindBuffer(gl.ARRAY_BUFFER, this.lineVerticesColorBuffer);
	gl.vertexAttribPointer(this.shaderProgram.vertexColorAttribute, 4, gl.FLOAT, false, 0, 0);

	// Draw the lines.
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.lineVerticesIndexBuffer);
	this.setMatrixUniforms();
	gl.drawElements(gl.LINES, this.lineVertexIndices.length, gl.UNSIGNED_SHORT, 0);
};

// Only needed for shapeView where the y-direction is backwards.
ms.view3d.prototype.swapY = function() {};
