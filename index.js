import { HSLSensorClient } from 'hslsensorclient'
import { HSLHttpServer } from 'hslhttpserver'

class HSLOpenSignalsLogger {
  constructor(sensorClient, filename) {
    this.hslClient = sensorClient;
    this.logFilename = filename;
  }

  open() {

  }

  close() {

  }
}

class HSLOpenSignalsGSRLogger extends HSLOpenSignalsLogger {
  constructor(sensorClient, filename) {
    super(sensorClient, filename)
  }

  open() {

  }

  close() {

  }
}

console.log('Started server at http://localhost:8090');
let hslClient = new HSLSensorClient();
let hslHttpServer = new HSLHttpServer(hslClient, 8090);
hslHttpServer.start()