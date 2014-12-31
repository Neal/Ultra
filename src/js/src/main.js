var Ultra = {};

Ultra.accessToken = localStorage.getItem('accessToken') || '';
Ultra.refreshToken = localStorage.getItem('refreshToken') || '';
Ultra.locations = JSON.parse(localStorage.getItem('locations')) || [];
Ultra.products = [];
Ultra.currentProduct = {};

Ultra.init = function () {
	Ultra.getTimeEstimates();
	setTimeout(function () { Keen.addEvent('init', { locations: { count: Ultra.locations.length }, hasAccessToken: (Ultra.accessToken.length !== 0) }); }, 100);
};

Ultra.sendError = function (type, error) {
	appMessageQueue.clear();
	appMessageQueue.send({
		type: type,
		method: METHOD.ERROR,
		name: error
	});
};

Ultra.getTimeEstimates = function () {
	Ultra.sendError(TYPE.PRODUCT, 'Getting geolocation...');
	navigator.geolocation.getCurrentPosition(function (pos) {
		var lat = pos.coords.latitude || 0;
		var lng = pos.coords.longitude || 0;
		Ultra.sendError(TYPE.PRODUCT, 'Requesting estimated pick up times...');
		if (!Ultra.accessToken) {
			var url = 'https://ineal.me/pebble/ultra/api/estimates/time';
			var data = { latitude: lat, longitude: lng };
			http('GET', url, data, null, function (e) {
				var res = JSON.parse(e.responseText);
				debugLog(res);
				if (!res.times || !res.times.length) {
					return Ultra.sendError(TYPE.PRODUCT, 'Uber is not yet available at this location or all cars are full at the moment. Please try again in a few minutes.');
				}
				Ultra.products = res.times;
				Ultra.sendProducts();
			}, function (e) {
				Ultra.sendError(TYPE.PRODUCT, e);
			});
			return;
		}
		UberAPI.timeEstimates(lat, lng, function (e) {
			var res = JSON.parse(e.responseText);
			debugLog(res);
			if (e.status === 401) {
				return Ultra.refreshAccessToken(Ultra.getTimeEstimates);
			}
			if (!res.times) return;
			if (!res.times.length) {
				return Ultra.checkProductAvailability(lat, lng);
			}
			Ultra.products = res.times;
			Ultra.sendError(TYPE.PRODUCT, 'Checking for surge pricing...');
			UberAPI.priceEstimates(lat, lng, lat, lng, function (e) {
				var res = JSON.parse(e.responseText);
				debugLog(res);
				if (res.prices && res.prices.length > 0) {
					res.prices.forEach(function (price) {
						for (var i = 0; i < Ultra.products.length; i++) {
							if (Ultra.products[i].product_id == price.product_id) {
								Ultra.products[i].surge_multiplier = price.surge_multiplier;
							}
						}
					});
				}
				Ultra.sendProducts();
			}, function (e) {
				Ultra.sendProducts();
			});
		}, function (e) {
			Ultra.sendError(TYPE.PRODUCT, e);
		});
	}, function (e) {
		Ultra.sendError(TYPE.PRODUCT, 'Error: unable to get geolocation!'); Keen.addEvent('locationFailed');
	}, { timeout: 10000 });
};

Ultra.getPriceEstimates = function () {
	var handleData = function (res, location, index) {
		if (!res.prices || !res.prices.length) return;
		var name = location.name.substring(0,12);
		var estimate = '';
		var duration = '';
		var distance = '';
		res.prices.forEach(function (price) {
			if (Ultra.currentProduct.product_id === price.product_id) {
				estimate = price.estimate;
				duration = Math.ceil(price.duration / 60) + ' min';
				distance = Math.ceil(price.distance);
				distance+= (distance > 1) ? ' miles' : ' mile';
			}
		});
		appMessageQueue.send({
			type: TYPE.LOCATION,
			method: METHOD.DATA,
			index: index,
			name: name,
			estimate: estimate,
			duration: duration,
			distance: distance,
		});
	};
	var sendNA = function (location, index) {
		appMessageQueue.send({
			type: TYPE.LOCATION,
			method: METHOD.DATA,
			index: index,
			name: location.name.substring(0,12),
			duration: 'N/A',
		});
	};
	navigator.geolocation.getCurrentPosition(function (pos) {
		var lat = pos.coords.latitude || 0;
		var lng = pos.coords.longitude || 0;
		var i = 0;
		if (!Ultra.accessToken) {
			Ultra.locations.forEach(function (location) {
				var index = i++;
				var url = 'https://ineal.me/pebble/ultra/api/estimates/price';
				var data = { start_latitude: lat, start_longitude: lng, end_latitude: location.lat, end_longitude: location.lng };
				http('GET', url, data, null, function (e) {
					if (e.status >= 400) {
						return sendNA(location, index);
					}
					var res = JSON.parse(e.responseText);
					debugLog(res);
					handleData(res, location, index);
				});
			});
			return;
		}
		Ultra.locations.forEach(function (location) {
			var index = i++;
			UberAPI.priceEstimates(lat, lng, location.lat, location.lng, function (e) {
				var res = JSON.parse(e.responseText);
				debugLog(res);
				if (e.status === 401) {
					return Ultra.refreshAccessToken(Ultra.getPriceEstimates);
				}
				if (res.code && res.code == 'distance_exceeded') {
					return sendNA(location, index);
				}
				handleData(res, location, index);
			});
		});
	}, function (e) {
		Ultra.sendError(TYPE.LOCATION, 'Error: unable to get geolocation!'); Keen.addEvent('locationFailed');
	}, { timeout: 10000 });
	Keen.addEvent('prices', { product: { name: Ultra.currentProduct.display_name }, hasAccessToken: (Ultra.accessToken.length !== 0) });
};

Ultra.sendLocations = function () {
	if (!Ultra.locations.length) {
		return Ultra.sendError(TYPE.LOCATION, 'No locations found. Use the app settings in the Pebble mobile app to add favorite locations.');
	}

	appMessageQueue.send({
		type: TYPE.LOCATION,
		method: METHOD.SIZE,
		index: Ultra.locations.length
	});

	for (var i = 0; i < Ultra.locations.length; i++) {
		appMessageQueue.send({
			type: TYPE.LOCATION,
			method: METHOD.DATA,
			index: i,
			name: Ultra.locations[i].name.substring(0,12),
		});
	}
};

Ultra.sendProducts = function () {
	appMessageQueue.send({
		type: TYPE.PRODUCT,
		method: METHOD.SIZE,
		index: Ultra.products.length
	});

	for (var i = 0; i < Ultra.products.length; i++) {
		var name = Ultra.products[i].display_name.substring(0,12);
		var estimate = Math.ceil(Ultra.products[i].estimate / 60) + ' min';
		var surge = (Ultra.products[i].surge_multiplier > 1) ? 'Surge pricing: ' + Ultra.products[i].surge_multiplier + 'x' : '';
		var resource = resources[name] || resources.uberX;
		appMessageQueue.send({
			type: TYPE.PRODUCT,
			method: METHOD.DATA,
			index: i,
			name: name,
			estimate: estimate,
			surge: surge,
			resource: resource-1,
		});
	}
};

Ultra.checkProductAvailability = function (lat, lng) {
	Ultra.sendError(TYPE.PRODUCT, 'Uber is not yet available at this location.');
	UberAPI.products(lat, lng, function (e) {
		var res = JSON.parse(e.responseText);
		debugLog(res);
		if (res.products && res.products.length > 0) {
			Ultra.sendError(TYPE.PRODUCT, 'All nearby cars are full but one should free up soon. Please try again in a few minutes.');
		}
	});
};

Ultra.refreshAccessToken = function (cb) {
	http('GET', 'https://ineal.me/pebble/ultra/api/refresh', { token: Ultra.refreshToken }, null, function (e) {
		var res = JSON.parse(e.responseText);
		debugLog(res);
		if (res.access_token) {
			Ultra.accessToken = res.access_token;
			localStorage.setItem('accessToken', Ultra.accessToken);
		}
		if (res.refresh_token) {
			Ultra.refreshToken = res.refresh_token;
			localStorage.setItem('refreshToken', Ultra.refreshToken);
		}
		if (typeof cb === 'function') cb();
	});
	Keen.addEvent('refreshToken');
};

Ultra.handleAppMessage = function (e) {
	console.log('AppMessage received: ' + JSON.stringify(e.payload));
	if (!e.payload.method) return;
	switch (e.payload.method) {
		case METHOD.REQUESTPRODUCTS:
			Ultra.getTimeEstimates();
			break;
		case METHOD.REQUESTLOCATIONS:
			Ultra.sendLocations();
			Ultra.currentProduct = Ultra.products[e.payload.index];
			Ultra.getPriceEstimates();
			break;
		case METHOD.REQUESTPRICE:
			Ultra.currentProduct = Ultra.products[e.payload.index];
			Ultra.getPriceEstimates();
			break;
	}
};

Ultra.showConfiguration = function () {
	var data = { locations: Ultra.locations };
	Keen.addEvent('configuration', { locations: { count: Ultra.locations.length }, hasAccessToken: (Ultra.accessToken.length !== 0) });
	Pebble.openURL('https://ineal.me/pebble/ultra/configuration/?data=' + encodeURIComponent(JSON.stringify(data)));
};

Ultra.handleConfiguration = function (e) {
	console.log('configuration received: ' + JSON.stringify(e));
	if (!e.response || e.response === 'CANCELLED') return;
	var data = JSON.parse(decodeURIComponent(e.response));
	if (data.keys) {
		if (data.keys.access_token) {
			Ultra.accessToken = data.keys.access_token;
			localStorage.setItem('accessToken', Ultra.accessToken);
		}
		if (data.keys.refresh_token) {
			Ultra.refreshToken = data.keys.refresh_token;
			localStorage.setItem('refreshToken', Ultra.refreshToken);
		}
		Ultra.getTimeEstimates();
	}
	if (data.locations) {
		Ultra.locations = data.locations;
		localStorage.setItem('locations', JSON.stringify(Ultra.locations));
		Ultra.sendLocations();
	}
};

var UberAPI = {};

UberAPI.products = function (lat, lng, cb, fb) {
	UberAPI.makeRequest('GET', '/v1/products', {latitude:lat, longitude:lng}, cb, fb);
};

UberAPI.timeEstimates = function (lat, lng, cb, fb) {
	UberAPI.makeRequest('GET', '/v1/estimates/time', {start_latitude:lat, start_longitude:lng}, cb, fb);
};

UberAPI.priceEstimates = function (start_lat, start_lng, end_lat, end_lng, cb, fb) {
	UberAPI.makeRequest('GET', '/v1/estimates/price', {start_latitude:start_lat, start_longitude:start_lng, end_latitude:end_lat, end_longitude:end_lng}, cb, fb);
};

UberAPI.makeRequest = function (method, endpoint, data, cb, fb) {
	if (!Ultra.accessToken) return Ultra.sendError(TYPE.PRODUCT, 'Please log in to your Uber account on the Pebble mobile app.');
	var url = 'https://api.uber.com' + endpoint;
	var headers = { 'Authorization': 'Bearer ' + Ultra.accessToken };
	http(method, url, data, headers, function (e) {
		if (typeof cb === 'function') cb(e);
	}, function (e) {
		if (typeof fb === 'function') fb(e);
	});
};

Pebble.addEventListener('ready', Ultra.init);
Pebble.addEventListener('appmessage', Ultra.handleAppMessage);
Pebble.addEventListener('showConfiguration', Ultra.showConfiguration);
Pebble.addEventListener('webviewclosed', Ultra.handleConfiguration);
