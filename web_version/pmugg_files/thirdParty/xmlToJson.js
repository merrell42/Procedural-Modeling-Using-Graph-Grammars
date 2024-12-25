ms.util.domParser = new DOMParser();

ms.util.xmlTextToJson = function(text) {
	var xmlDoc = ms.util.domParser.parseFromString(text,"text/xml");
	return ms.util.xmlToJson(xmlDoc);
};

// https://davidwalsh.name/convert-xml-json
// Changes XML to JSON
ms.util.xmlToJson = function(xml) {
	
	// Create the return object
	var obj = {};

	if (xml.nodeType == 1) { // element
		// do attributes
		if (xml.attributes.length > 0) {
			obj["@attributes"] = {};
			for (var j = 0; j < xml.attributes.length; j++) {
				var attribute = xml.attributes.item(j);
				obj["@attributes"][attribute.nodeName] = attribute.nodeValue;
				// obj[attribute.nodeName] = attribute.nodeValue;
			}
		}
	} else if (xml.nodeType == 3) { // text
		obj = xml.nodeValue;
	}

	// do children
	if (xml.hasChildNodes()) {
		for(var i = 0; i < xml.childNodes.length; i++) {
			var item = xml.childNodes.item(i);
			var nodeName = item.nodeName;
			if (nodeName == '#text') {
				continue;
			}
			if (typeof(obj[nodeName]) == "undefined") {
				obj[nodeName] = ms.util.xmlToJson(item);
			} else {
				if (typeof(obj[nodeName].push) == "undefined") {
					var old = obj[nodeName];
					obj[nodeName] = [];
					obj[nodeName].push(old);
				}
				obj[nodeName].push(ms.util.xmlToJson(item));
			}
		}
	}
	return obj;
};

// https://stackoverflow.com/questions/48788722/json-to-xml-using-javascript
ms.util.objToXml = function(obj) {
	var xml = '';
	for (var prop in obj) {
		if (prop == '@attributes') {
			continue;
		}
		var attributes = '';
		if (obj[prop]['@attributes']) {
			var attrProps = obj[prop]['@attributes'];
			Object.keys(attrProps).forEach((key) => {
				attributes += ' ' + key + '=\'' + attrProps[key] + '\'';
			});
		}
		xml += obj[prop] instanceof Array ? '' : "<" + prop + attributes + ">";
		if (obj[prop] instanceof Array) {
			for (var array in obj[prop]) {				
				var attributes = '';
				if (obj[prop][array]['@attributes']) {
					var attrProps = obj[prop][array]['@attributes'];
					Object.keys(attrProps).forEach((key) => {
						attributes += ' ' + key + '=\'' + attrProps[key] + '\'';
					});
				}
				xml += "<" + prop + attributes + ">";
				xml += ms.util.objToXml(new Object(obj[prop][array]));
				xml += "</" + prop + ">";
			}
		} else if (typeof obj[prop] == "object") {
			xml += ms.util.objToXml(new Object(obj[prop]));
		} else {
			xml += obj[prop];
		}
		xml += obj[prop] instanceof Array ? '' : "</" + prop + ">";
	}
	var xml = xml.replace(/<\/?[0-9]{1,}>/g, '');
	return xml
}