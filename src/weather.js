Pebble.addEventListener('ready', function (e) {
    console.log('PebbleKit JS ready!');
    getWeather();
  }
);

Pebble.addEventListener('appmessage', function (e) {
    console.log('AppMessage received!');
    getWeather();
  }                     
);

var doXhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  
  xhr.onload = function () {
    callback(this.responseText);
  };
  
  xhr.open(type, url);
  xhr.send();
};


function requestWeather(position) {
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
    position.coords.latitude + '&lon=' + position.coords.longitude;
  doXhrRequest(url, 'GET', function (responseText) {
    var json = JSON.parse(responseText),
      temperature = Math.round(json.main.temp - 273.15),
      conditions = json.weather[0].main;      
    
    Pebble.sendAppMessage({
        'KEY_TEMPERATURE': temperature,
        'KEY_CONDITIONS': conditions
      }, function (e) {
        console.log('Weather info sent to Pebble successfully!');
      }, function (e) {
        console.log('Error sending weather info to Pebble!');
      }
    );
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    requestWeather,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}