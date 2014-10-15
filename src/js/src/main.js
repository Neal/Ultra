var Uber = {
	accessToken: localStorage.getItem('accessToken') || '',
	refreshToken: localStorage.getItem('refreshToken') || '',
	locations: JSON.parse(localStorage.getItem('locations')) || [],
	products: [],

	init: function() {
		Uber.refresh();
	},

	sendError: {
		product: function(error) {
			appMessageQueue.clear();
			appMessageQueue.send({type:TYPE.PRODUCT, method:METHOD.ERROR, name:error});
		},
		location: function(error) {
			appMessageQueue.clear();
			appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.ERROR, name:error});
		}
	},

	error: function(error) {
		appMessageQueue.clear();
		appMessageQueue.send({type:TYPE.ERROR, name:error});
	},

	sendLocations: function() {
		if (!Uber.locations.length) {
			return Uber.sendError.location('No locations found. Use the app settings in the Pebble mobile app to add favorite locations.');
		}
		appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.SIZE, index:Uber.locations.length});
		for (var i = 0; i < Uber.locations.length; i++) {
			var name = Uber.locations[i].name.substring(0,12);
			appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.DATA, index:i, name:name, estimate:'', duration:'', distance:''});
		}
	},

	sendProducts: function() {
		appMessageQueue.send({type:TYPE.PRODUCT, method:METHOD.SIZE, index:Uber.products.length});
		for (var i = 0; i < Uber.products.length; i++) {
			var name = Uber.products[i].display_name.substring(0,12);
			var estimate = Math.ceil(Uber.products[i].estimate / 60) + ' min';
			var surge = (Uber.products[i].surge_multiplier > 1) ? 'Surge pricing: ' + Uber.products[i].surge_multiplier + 'x' : '';
			var resource = resources[name] || resources.uberX;
			appMessageQueue.send({type:TYPE.PRODUCT, method:METHOD.DATA, index:i, name:name, estimate:estimate, surge:surge, resource:resource-1});
		}
	},

	checkProducts: function(latitude, longitude) {
		Uber.sendError.product('Uber is not yet available at this location.');
		Uber.api.products(latitude, longitude, function(xhr) {
			var res = JSON.parse(xhr.responseText);
			console.log(JSON.stringify(res));
			if (res.products && res.products.length > 0) {
				Uber.sendError.product('All nearby cars are full but one should free up soon. Please try again in a few minutes.');
			}
		});
	},

	getPriceEstimates: function(productIndex) {
		navigator.geolocation.getCurrentPosition(function(pos) {
			var latitude = pos.coords.latitude || 0;
			var longitude = pos.coords.longitude || 0;
			var i = 0;
			if (!Uber.accessToken) {
				Uber.locations.forEach(function(location) {
					var index = i++;
					var url = 'https://ineal.me/pebble/ultra/api/estimates/price?start_latitude=' + latitude + '&start_longitude=' + longitude + '&end_latitude=' + location.lat + '&end_longitude=' + location.lng;
					var xhr = new XMLHttpRequest();
					xhr.open('GET', url, true);
					xhr.onload = function() {
						var res = JSON.parse(xhr.responseText);
						console.log(JSON.stringify(res));
						if (!res.prices || !res.prices.length) return;
						var name = location.name.substring(0,12);
						var estimate = '';
						var duration = '';
						var distance = '';
						res.prices.forEach(function(price) {
							if (Uber.products[productIndex].product_id == price.product_id) {
								estimate = price.estimate;
								duration = Math.ceil(price.duration / 60) + ' min';
								distance = Math.ceil(price.distance);
								distance+= (distance > 1) ? ' miles' : ' mile';
							}
						});
						appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.DATA, index:index, name:name, estimate:estimate, duration:duration, distance:distance});
					};
					xhr.send(null);
				});
				return;
			}
			Uber.locations.forEach(function(location) {
				var index = i++;
				Uber.api.priceEstimates(latitude, longitude, location.lat, location.lng, function(xhr) {
					var res = JSON.parse(xhr.responseText);
					console.log(JSON.stringify(res));
					if (xhr.status === 401) {
						return Uber.refreshAccessToken(Uber.getPriceEstimates);
					}
					if (res.code && res.code == 'distance_exceeded') {
						return appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.DATA, index:index, name:location.name.substring(0,12), estimate:'', duration:'N/A', distance:''});
					}
					if (!res.prices || !res.prices.length) return;
					var name = location.name.substring(0,12);
					var estimate = '';
					var duration = '';
					var distance = '';
					res.prices.forEach(function(price) {
						if (Uber.products[productIndex].product_id == price.product_id) {
							estimate = price.estimate;
							duration = Math.ceil(price.duration / 60) + ' min';
							distance = Math.ceil(price.distance);
							distance+= (distance > 1) ? ' miles' : ' mile';
						}
					});
					appMessageQueue.send({type:TYPE.LOCATION, method:METHOD.DATA, index:index, name:name, estimate:estimate, duration:duration, distance:distance});
				});
			});
		}, function(err) { Uber.sendError.location('Failed to get geolocation!'); }, { timeout: 10000 });
	},

	refresh: function() {
		Uber.sendError.product('Trying to get Geolocation...');
		navigator.geolocation.getCurrentPosition(function(pos) {
			var latitude = pos.coords.latitude || 0;
			var longitude = pos.coords.longitude || 0;
			Uber.sendError.product('Requesting estimated pick up times...');
			if (!Uber.accessToken) {
				var url = 'https://ineal.me/pebble/ultra/api/estimates/time?latitude=' + latitude + '&longitude=' + longitude;
				var xhr = new XMLHttpRequest();
				xhr.open('GET', url, true);
				xhr.onload = function() {
					var res = JSON.parse(xhr.responseText);
					console.log(JSON.stringify(res));
					if (!res.times || !res.times.length) {
						return Uber.sendError.product('Uber is not yet available at this location or all cars are full at the moment. Please try again in a few minutes.');
					}
					Uber.products = res.times;
					Uber.sendProducts();
				};
				xhr.onerror = function() { Uber.sendError.product('Connection error!'); };
				xhr.ontimeout = function() { Uber.sendError.product('Connection to Uber API timed out!'); };
				xhr.timeout = 30000;
				xhr.send(null);
				return;
			}
			Uber.api.timeEstimates(latitude, longitude, function(xhr) {
				var res = JSON.parse(xhr.responseText);
				console.log(JSON.stringify(res));
				if (xhr.status === 401) {
					return Uber.refreshAccessToken(Uber.refresh);
				}
				if (!res.times) return;
				if (!res.times.length) {
					return Uber.checkProducts(latitude, longitude);
				}
				Uber.products = res.times;
				Uber.sendError.product('Checking for surge pricing...');
				Uber.api.priceEstimates(latitude, longitude, latitude, longitude, function(xhr2) {
					var res2 = JSON.parse(xhr2.responseText);
					console.log(JSON.stringify(res2));
					if (res2.prices && res2.prices.length > 0) {
						res2.prices.forEach(function(price) {
							for (var i = 0; i < Uber.products.length; i++) {
								if (Uber.products[i].product_id == price.product_id) {
									Uber.products[i].surge_multiplier = price.surge_multiplier;
								}
							}
						});
					}
					Uber.sendProducts();
				}, function(err) {
					Uber.sendProducts();
				});
			}, Uber.sendError.product);
		}, function(err) { Uber.sendError.product('Failed to get geolocation!'); }, { timeout: 10000 });
	},

	api: {
		products: function(latitude, longitude, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/products', serialize({latitude:latitude, longitude:longitude}), cb, fb);
		},

		timeEstimates: function(latitude, longitude, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/estimates/time', serialize({start_latitude:latitude, start_longitude:longitude}), cb, fb);
		},

		priceEstimates: function(start_lat, start_lng, end_lat, end_lng, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/estimates/price', serialize({start_latitude:start_lat, start_longitude:start_lng, end_latitude:end_lat, end_longitude:end_lng}), cb, fb);
		},

		makeRequest: function(method, endpoint, data, cb, fb) {
			if (!Uber.accessToken) return Uber.sendError.product('Please log in to your Uber account on the Pebble mobile app.');
			var url = 'https://api.uber.com' + endpoint;
			if (method == 'GET' && data) {
				url += '?' + data;
				data = null;
			}
			console.log(method + ' ' + url + ' ' + data);
			var xhr = new XMLHttpRequest();
			xhr.setRequestHeader('Authorization', 'Bearer ' + Uber.accessToken);
			xhr.open(method, url, true);
			xhr.onload = function() { cb(xhr); };
			xhr.onerror = function() { fb('Connection error!'); };
			xhr.ontimeout = function() { fb('Connection to Uber API timed out!'); };
			xhr.timeout = 30000;
			xhr.send(data);
		}
	},

	handleAppMessage: function(e) {
		console.log('AppMessage received: ' + JSON.stringify(e.payload));
		if (!e.payload.method) return;
		switch (e.payload.method) {
			case METHOD.REQUESTPRODUCTS:
				Uber.refresh();
				break;
			case METHOD.REQUESTLOCATIONS:
				Uber.sendLocations();
				Uber.getPriceEstimates(e.payload.index);
				break;
			case METHOD.REQUESTPRICE:
				Uber.getPriceEstimates(e.payload.index);
				break;
		}
	},

	refreshAccessToken: function(cb) {
		var xhr = new XMLHttpRequest();
		xhr.open('GET', 'https://ineal.me/pebble/ultra/api/refresh?token=' + Uber.refreshToken, true);
		xhr.onload = function() {
			var res = JSON.parse(xhr.responseText);
			console.log('new tokens: ' + JSON.stringify(res));
			if (res.access_token) {
				Uber.accessToken = res.access_token;
				localStorage.setItem('accessToken', Uber.accessToken);
			}
			if (res.refresh_token) {
				Uber.refreshToken = res.refresh_token;
				localStorage.setItem('refreshToken', Uber.refreshToken);
			}
			if (cb && typeof(cb) === 'function') {
				cb();
			}
		};
		xhr.send(null);
	},

	showConfiguration: function() {
		var data = {locations: Uber.locations};
		Pebble.openURL('https://ineal.me/pebble/ultra/configuration/?data=' + encodeURIComponent(JSON.stringify(data)));
	},

	handleConfiguration: function(e) {
		console.log('configuration received: ' + JSON.stringify(e));
		if (!e.response) return;
		var data = JSON.parse(decodeURIComponent(e.response));
		if (data.keys) {
			if (data.keys.access_token) {
				Uber.accessToken = data.keys.access_token;
				localStorage.setItem('accessToken', Uber.accessToken);
			}
			if (data.keys.refresh_token) {
				Uber.refreshToken = data.keys.refresh_token;
				localStorage.setItem('refreshToken', Uber.refreshToken);
			}
			Uber.refresh();
		}
		if (data.locations) {
			Uber.locations = data.locations;
			localStorage.setItem('locations', JSON.stringify(Uber.locations));
			Uber.sendLocations();
		}
	}
};

Pebble.addEventListener('ready', Uber.init);
Pebble.addEventListener('appmessage', Uber.handleAppMessage);
Pebble.addEventListener('showConfiguration', Uber.showConfiguration);
Pebble.addEventListener('webviewclosed', Uber.handleConfiguration);
