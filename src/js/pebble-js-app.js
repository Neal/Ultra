var TYPE = {
	ERROR: 0,
	PRODUCT: 1
};

var METHOD = {
	SIZE: 0,
	DATA: 1,
	REFRESH: 2,
	READY: 3
};

var Uber = {
	accessToken: localStorage.getItem('accessToken') || '',
	refreshToken: localStorage.getItem('refreshToken') || '',

	init: function() {
		Uber.refresh();
	},

	error: function(error) {
		appMessageQueue.clear();
		appMessageQueue.send({type:TYPE.ERROR, name:error});
	},

	sendProducts: function(products) {
		appMessageQueue.send({type:TYPE.PRODUCT, method:METHOD.SIZE, index:products.length});
		for (var i = 0; i < products.length; i++) {
			var name = products[i].display_name.substring(0,12);
			var estimate = Math.ceil(products[i].estimate / 60) + ' min';
			var surge_multiplier = products[i].surge_multiplier || 1;
			var surge = (surge_multiplier > 1) ? 'Surge pricing: ' + surge_multiplier + 'x' : '';
			var resource = 0;
			switch (name.toLowerCase()) {
				case 'uberx': resource = 0; break;
				case 'uberxl': resource = 1; break;
				case 'uberblack': resource = 2; break;
				case 'ubersuv': resource = 3; break;
				case 'ubertaxi': resource = 4; break;
				case 'ubert': resource = 5; break;
			}
			appMessageQueue.send({type:TYPE.PRODUCT, method:METHOD.DATA, index:i, name:name, estimate:estimate, surge:surge, resource:resource});
		}
	},

	checkProducts: function(latitude, longitude) {
		Uber.error('Uber is not yet available at this location.');
		Uber.api.products(latitude, longitude, function(xhr) {
			var res = JSON.parse(xhr.responseText);
			console.log(JSON.stringify(res));
			if (res.products && res.products.length > 0) {
				Uber.error('All nearby cars are full but one should free up soon. Please try again in a few minutes.');
			}
		});
	},

	refresh: function() {
		Uber.error('Trying to get Geolocation...');
		navigator.geolocation.getCurrentPosition(function(pos) {
			var latitude = pos.coords.latitude || 0;
			var longitude = pos.coords.longitude || 0;
			Uber.error('Requesting estimated pick up times...');
			if (!Uber.accessToken) {
				var url = 'https://ineal.me/pebble/ultra/api/estimates?latitude=' + latitude + '&longitude=' + longitude;
				var xhr = new XMLHttpRequest();
				xhr.open('GET', url, true);
				xhr.onload = function() {
					console.log(xhr.responseText);
					var res = JSON.parse(xhr.responseText);
					console.log(JSON.stringify(res));
					if (!res.times) return;
					if (!res.times.length) {
						return Uber.error('Uber is not yet available at this location or all cars are full at the moment. Please try again in a few minutes.');
					}
					Uber.sendProducts(res.times);
				};
				xhr.onerror = function() { Uber.error('Connection error!'); };
				xhr.ontimeout = function() { Uber.error('Connection to Uber API timed out!'); };
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
				Uber.error('Checking for surge pricing...');
				Uber.api.priceEstimates(latitude, longitude, function(xhr2) {
					console.log('1');
					var res2 = JSON.parse(xhr2.responseText);
					console.log(JSON.stringify(res2));
					if (res2.prices && res2.prices.length > 0) {
						res2.prices.forEach(function(price) {
							for (var i = 0; i < res.times.length; i++) {
								if (res.times[i].product_id == price.product_id) {
									res.times[i].surge_multiplier = price.surge_multiplier;
								}
							}
						});
					}
					Uber.sendProducts(res.times);
				}, function(err) {
					Uber.sendProducts(res.times);
				});
			}, Uber.error);
		}, function(err) { Uber.error('Failed to get geolocation!'); }, { timeout: 10000 });
	},

	api: {
		products: function(latitude, longitude, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/products', serialize({latitude:latitude, longitude:longitude}), cb, fb);
		},

		timeEstimates: function(latitude, longitude, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/estimates/time', serialize({start_latitude:latitude, start_longitude:longitude}), cb, fb);
		},

		priceEstimates: function(latitude, longitude, cb, fb) {
			Uber.api.makeRequest('GET', '/v1/estimates/price', serialize({start_latitude:latitude, start_longitude:longitude, end_latitude:latitude, end_longitude:longitude}), cb, fb);
		},

		makeRequest: function(method, endpoint, data, cb, fb) {
			if (!Uber.accessToken) return Uber.error('Please log in to your Uber account on the Pebble mobile app.');
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
			case METHOD.REFRESH:
				Uber.refresh();
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
		Pebble.openURL('https://ineal.me/pebble/ultra/api/login');
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
	}
};

Pebble.addEventListener('ready', Uber.init);
Pebble.addEventListener('appmessage', Uber.handleAppMessage);
Pebble.addEventListener('showConfiguration', Uber.showConfiguration);
Pebble.addEventListener('webviewclosed', Uber.handleConfiguration);

var appMessageQueue = {
	queue: [],
	numTries: 0,
	maxTries: 5,
	working: false,
	clear: function() {
		this.queue = [];
		this.working = false;
	},
	isEmpty: function() {
		return this.queue.length === 0;
	},
	nextMessage: function() {
		return this.isEmpty() ? {} : this.queue[0];
	},
	send: function(message) {
		if (message) this.queue.push(message);
		if (this.working) return;
		if (this.queue.length > 0) {
			this.working = true;
			var ack = function() {
				appMessageQueue.numTries = 0;
				appMessageQueue.queue.shift();
				appMessageQueue.working = false;
				appMessageQueue.send();
			};
			var nack = function() {
				appMessageQueue.numTries++;
				appMessageQueue.working = false;
				appMessageQueue.send();
			};
			if (this.numTries >= this.maxTries) {
				console.log('Failed sending AppMessage: ' + JSON.stringify(this.nextMessage()));
				ack();
			}
			console.log('Sending AppMessage: ' + JSON.stringify(this.nextMessage()));
			Pebble.sendAppMessage(this.nextMessage(), ack, nack);
		}
	}
};

function serialize(obj) {
	var s = [];
	for (var p in obj) {
		if (obj.hasOwnProperty(p)) {
			s.push(encodeURIComponent(p) + '=' + encodeURIComponent(obj[p]));
		}
	}
	return s.join('&');
}
