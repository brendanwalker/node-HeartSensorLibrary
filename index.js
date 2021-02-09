var hsl = require("./build/Debug/heartsensorlibrary.node");
module.exports = hsl;

var url = require('url');
var fs = require('fs');
var path = require('path');
var baseDirectory = path.join(__dirname, "public");
var net = require('net');
var http = require('http');
// .extend adds a .withShutdown prototype method to the Server object
require('http-shutdown').extend();

class HSLSensorClient {
  constructor() {
    this.updateInternal = null;
    this.sensors = [];
    this.listenerCallbacks = [];
  }

  refreshSensorList() {
    let sensorList = hsl.getSensorList();

    this.sensors = [];
    for (var i = 0; i < sensorList.getSensorCount(); i++) {
      let sensor = sensorList.getSensor(i);

      if (sensor != null) {
        this.sensors.push(sensor);
        if (sensor.hasCapability(hsl.Sensor.StreamFlags_ECGData))
          sensor.setDataStreamActive(hsl.Sensor.StreamFlags_ECGData, true)
        else if (sensor.hasCapability(hsl.Sensor.StreamFlags_PPGData))
          sensor.setDataStreamActive(hsl.Sensor.StreamFlags_PPGData, true)
        else if (sensor.hasCapability(hsl.Sensor.StreamFlags_HRData))
          sensor.setDataStreamActive(hsl.Sensor.StreamFlags_HRData, true)
      }
    }
  }

  addListener(_this, callback_fn) {
    this.listenerCallbacks.push(callback_fn.bind(_this));
  }

  removeListener(callback_fn) {
    var index = this.listenerCallbacks.indexOf(callback_fn);
    if (index >= 0) {
      this.listenerCallbacks.splice(index, 1);
    }
  }

  publishSensorECGStream(sensor) {
    var ecgBufferIter = sensor.getHeartECGBuffer();

    var ecgArray = [];
    while (ecgBufferIter.isValid()) {
      ecgArray.push(ecgBufferIter.getECGData());
      ecgBufferIter.next();
    }

    if (ecgArray.length > 0) {
      sensor.flushHeartECGBuffer();

      publishData({ id: sensor.getSensorID(), type: "ecg", stream: ecgArray });
    }
  }

  publishSensorPPGStream(sensor) {
    var ppgBufferIter = sensor.getHeartPPGBuffer();

    var ppgArray = [];
    while (ppgBufferIter.isValid()) {
      ppgArray.push(ppgBufferIter.getPPGData());
      ppgBufferIter.next();
    }

    if (ppgArray.length > 0) {
      sensor.flushHeartPPGBuffer();

      this.publishData({ id: sensor.getSensorID(), type: "ppg", stream: ppgArray });
    }
  }

  publishSensorHRStream(sensor) {
    var hrBufferIter = sensor.getHeartRateBuffer();

    var hrArray = [];
    while (hrBufferIter.isValid()) {
      hrArray.push(hrBufferIter.getHRData());
      hrBufferIter.next();
    }

    if (hrArray.length > 0) {
      sensor.flushHeartRateBuffer();

      publishData({ id: sensor.getSensorID(), type: "hr", stream: hrArray });
    }
  }

  publishData(sensorData) {
    this.listenerCallbacks.forEach(function (callback_fn) {
      callback_fn(sensorData);
    });
  }

  update() {
    hsl.update();

    if (hsl.hasSensorListChanged()) {
      this.refreshSensorList();
    }

    var _this = this;
    this.sensors.forEach(function (sensor) {
      _this.publishSensorECGStream(sensor);
      _this.publishSensorPPGStream(sensor);
      _this.publishSensorHRStream(sensor);
    });
  }

  start() {
    if (this.updateInternal == null) {
      console.log("HSL " + hsl.getVersionString());

      var _this = this;
      this.updateInternal = setInterval(function () { _this.update(); }, 100);
    }
  }

  stop() {
    if (this.updateInternal != null) {
      this.sensors.forEach(function (sensor) {
        sensor.stopAllStreams();
      });

      clearInterval(this.updateInternal);
      this.updateInternal = null;
    }
  }
}

class HSLHttpServer {
  constructor(port) {
    //  For the static files we server out of the 
    this.contentTypeByExtension = {
      '.css': 'text/css',
      '.gif': 'image/gif',
      '.html': 'text/html',
      '.jpg': 'image/jpeg',
      '.js': 'text/javascript',
      '.png': 'image/png',
    };

    this.httpServer = null;
    this.httpPort = port;
    this.hslClient = new HSLSensorClient();

    // This array holds the clients (actually http server response objects) to send data to over SSE
    this.clients = [];
  }

  handleServerStreamEventRequest(request, response) {
    // Return Server-Stream-Event data
    // http://www.html5rocks.com/en/tutorials/eventsource/basics/
    var headers = {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive'
    };
    response.writeHead(200, headers);

    this.addClient(response);
  }

  handleStaticContentRequest(request, response) {
    // Technique modified from this:
    // http://stackoverflow.com/questions/6084360/using-node-js-as-a-simple-web-server
    const baseURL = 'http://' + request.headers.host + '/';
    const requestUrl = new URL(request.url, baseURL);

    // path.normalize prevents using .. to go above the base directory
    var pathname = path.normalize(requestUrl.pathname);

    // Allows http://localhost:8080/ to be used as the URL to retrieve the main index page
    if (pathname == '/' || pathname == '\\') {
      pathname = 'index.html';
    }

    // Handle static file like index.html and main.js

    // Include an appropriate content type for known files like .html, .js, .css
    var headers = {};
    var contentType = this.contentTypeByExtension[path.extname(pathname)];
    if (contentType) {
      headers['Content-Type'] = contentType;
    }

    // path.normalize prevents using .. to go above the base directory above, so
    // this can only serve files in the public directory
    var fsPath = path.join(baseDirectory, pathname);
    var fileStream = fs.createReadStream(fsPath);

    response.writeHead(200, headers);
    fileStream.pipe(response);
    fileStream.on('error', function (e) {
      response.writeHead(404);
      response.end();
    });
  }

  handleRequest(request, response) {
    try {
      // Technique modified from this:
      // http://stackoverflow.com/questions/6084360/using-node-js-as-a-simple-web-server
      const baseURL = 'http://' + request.headers.host + '/';
      const requestUrl = new URL(request.url, baseURL);

      // path.normalize prevents using .. to go above the base directory
      var pathname = path.normalize(requestUrl.pathname);

      if (pathname == '/data' || pathname == '\\data') {
        this.handleServerStreamEventRequest(request, response)
      }
      else {
        this.handleStaticContentRequest(request, response)
      }
    } catch (e) {
      response.writeHead(500);
      response.end();
      console.log(e.stack);
    }
  }

  addClient(client) {
    this.clients.push(client);
  }

  removeClient(client) {
    var index = this.clients.indexOf(client);
    if (index >= 0) {
      this.clients.splice(index, 1);
    }
  }

  // Send data to all SSE web browser clients. data must be a string.
  handleSensorData(data) {
    var failures = [];

    this.clients.forEach(function (client) {
      let dataStr = JSON.stringify(data)
      if (!client.write('data: ' + dataStr + '\n\n')) {
        failures.push(client);
      }
    });

    var _this = this;
    failures.forEach(function (client) {
      console.log("[ERROR] Stopping server stream event");
      _this.removeClient(client);
      client.end();
    });
  }

  start() {
    var _this = this;
    this.httpServer = http.createServer(function (request, response) {
      _this.handleRequest(request, response);
    }).withShutdown();
    this.httpServer.listen(this.httpPort);

    this.hslClient.addListener(this, this.handleSensorData);
    this.hslClient.start();
  }

  stop() {
    var _this = this;
    this.httpServer.shutdown(function () {
      _this.hslClient.stop();
      _this.httpServer = null;
    });
  }
}

console.log('Started server at http://localhost:8090');
let hslHttpServer = new HSLHttpServer(8090);
hslHttpServer.start()