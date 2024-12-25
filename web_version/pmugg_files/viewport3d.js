ms.viewport3d = function() {
	this.fov = 45;
	this.theta = 0.2 * Math.PI;
	this.phi = -0.5 * Math.PI;;
	
	this.eye = this.getEyeDir().scale(-10);
	this.up = new ms.vec3(0, 0, 1);
	this.interest = new ms.vec3(0, 0, 0);
	this.intScreen = new ms.vec2(0, 0);

	// Move the eye closer to the center of the model.
	this.eye.move(1.5, 0.8, 0);
	this.interest.move(1.5, 0.8, 0);

	this.width = 0;
	this.height = 0;
};

ms.viewport3d.prototype.resize = function(w, h) {
	this.width = w;
	this.height = h;
};

ms.viewport3d.prototype.getFov = function() {
	return this.fov;
};

// From spherical coordinates, return a cartesian coordinates.
ms.viewport3d.sphereToCart = function(r, theta, phi) {
	return new ms.vec3(
		r * Math.sin(theta) * Math.cos(phi),
		r * Math.sin(theta) * Math.sin(phi),
		r * Math.cos(theta)
	);
};

ms.viewport3d.prototype.getEyeDir = function() {
	return ms.viewport3d.sphereToCart(-1, this.theta, this.phi);
};

ms.viewport3d.prototype.getEye = function() {
	return this.eye;
};

ms.viewport3d.prototype.getTarget = function() {
	return this.getEyeDir().add(this.eye);
};

ms.viewport3d.prototype.getUp = function() {
	return this.up;
};

// u and v point in the x-screen and y-screen directions.
ms.viewport3d.prototype.getUV = function() {
	var eyeDir = this.getEyeDir();
	var u = eyeDir.cross(this.up).normalize();
	var v = u.cross(eyeDir).normalize();
	return [u, v];
}

ms.viewport3d.prototype.move = function(dx, dy) {
	var [u, v] = this.getUV();
	u.scale(dx / 100);
	v.scale(-dy / 100);
	this.eye.add(u);
	this.eye.add(v);
};

ms.viewport3d.prototype.orbit = function(dx, dy) {
	var deltaPhi = -dx * Math.PI / 400;
	var deltaTheta = -dy * Math.PI / 400;
	this.theta += deltaTheta;
	this.theta = Math.max(0.01, this.theta);
	this.theta = Math.min(this.theta, Math.PI);
	this.phi += deltaPhi;

	// v is a vector from interest point to the eye.
	var v = this.eye.copy().minus(this.interest);
	var r = v.length();
	var ray = this.screenRay(this.intScreen.x, this.intScreen.y);
	ray.normalize().scale(-r);
	this.eye = this.interest.copy().add(ray);
};

ms.viewport3d.prototype.zoom = function(dir, focusX, focusY) {
	this.setInterest(focusX, focusY);
	var eyeToInterest = this.eye.copy().minus(this.interest);
	if (dir == 1) {
		eyeToInterest.scale(1 / ms.mainController.ZOOM_AMOUNT);
	} else {
		eyeToInterest.scale(ms.mainController.ZOOM_AMOUNT);
	}
	this.eye = eyeToInterest.add(this.interest);
};

ms.viewport3d.prototype.rayToGround = function(ray) {	
	var eye = this.getEye();
	var d = -eye.z / ray.z;
	return new ms.vec3(eye.x + d * ray.x, eye.y + d * ray.y, 0);
};

// Returns the direction of a ray from the camera center to a position on the screen.
ms.viewport3d.prototype.screenRay = function(x, y) {
	var w2 = this.width / 2;
	var h2 = this.height / 2;

	var eyeDir = this.getEyeDir();
	var [u, v] = this.getUV();
	var su = (x - w2) / h2 * Math.tan(this.fov / 2 * Math.PI / 180);
	var sv = (h2 - y) / h2 * Math.tan(this.fov / 2 * Math.PI / 180);
	u.scale(su);
	v.scale(sv);
	return eyeDir.add(u).add(v);
};


ms.viewport3d.prototype.setInterest = function(x, y) {
	var ray = this.screenRay(x, y);
	this.interest = this.rayToGround(ray);
	this.intScreen = new ms.vec2(x, y);
};
