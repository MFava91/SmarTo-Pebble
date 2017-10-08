var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var smartoStatus = {
  code: 4,
  status: 'Fetching data...'
}

function checkAvailability(value, timeOn) {
  if(value == 0) {
    if(timeOn < 1200) {
      smartoStatus.code = 0;
      smartoStatus.status = 'Occupied';
    }
    else {
      smartoStatus.code = 2;
      smartoStatus.status = 'Maybe available';
    }
  } else {
      smartoStatus.code = 1;
      smartoStatus.status = 'Available';
  }
}

function fetchSmarToStatus() {
  var req = new XMLHttpRequest();
  req.open('GET', 'http://' + localStorage.getItem('SMARTO_SERVER_IP') + '/status', true);
  req.timeout = 2500;
  req.onload = function() {
    if (req.status === 200) {
      var response = JSON.parse(req.responseText);
      var lightSensor = response.lightSensor.value;
      var lightSensorTimeOn = response.lightSensor.timeOn;
      checkAvailability(lightSensor, lightSensorTimeOn);
      Pebble.sendAppMessage({
          'SMARTO_STATUS_ICON_KEY': smartoStatus.code,
          'SMARTO_STATUS_TEXT_KEY': smartoStatus.status
      });
    } else {
      sendErrorMessage();
    }
  };

  req.ontimeout = function() {
    console.log('TIMEOUT');
    Pebble.sendAppMessage({
      'SMARTO_STATUS_ICON_KEY': 4,
      'SMARTO_STATUS_TEXT_KEY': 'Timeout',
      'SMARTO_BOOKING_KEY': 'check your config'
    });
  }

  req.onerror = function() {
    sendErrorMessage();
  }

  req.send(null);
}

function fetchIfBooked() {
  var req = new XMLHttpRequest();
    req.open('GET', 'http://' + localStorage.getItem('SMARTO_SERVER_IP') + '/booking', true);
    req.onload = function () {
      if (req.status === 200) {
        var bookingResponse = JSON.parse(req.responseText);
        if(bookingResponse.length > 20) {
          bookingResponse = bookingResponse.substring(0, 20);
        }
        Pebble.sendAppMessage({
          'SMARTO_BOOKING_KEY': bookingResponse
        });
      } else {
        
      }
    };
    req.onerror = function() {
      //console.log(req.status);
      sendErrorMessage();
    }
    req.send(null);
}

function checkIpAndFetchData() {
  if (localStorage.getItem('SMARTO_SERVER_IP') != null) {
    fetchSmarToStatus();
    fetchIfBooked();
  } else {
    sendErrorMessage();
  }
}

function sendErrorMessage() {
  Pebble.sendAppMessage({
      'SMARTO_STATUS_ICON_KEY': 4,
      'SMARTO_STATUS_TEXT_KEY': '',
      'SMARTO_BOOKING_KEY': 'Error loading status!'
    });
}

// Pebble event listener functions
Pebble.addEventListener('ready', function (e) {
  checkIpAndFetchData();
  setInterval(function(){
      checkIpAndFetchData();
  }, 5000)
});

Pebble.addEventListener('webviewclosed', function (e) {
  var config = JSON.parse(e.response);
  localStorage.setItem('SMARTO_SERVER_IP', config.SMARTO_SERVER_IP.value);
  checkIpAndFetchData();
});
