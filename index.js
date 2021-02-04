var hsl = require("./build/Debug/heartsensorlibrary.node");
module.exports = hsl;

const readline = require('readline');
readline.emitKeypressEvents(process.stdin);
if (process.stdin.isTTY) {
  process.stdin.setRawMode(true);
}

class HSLConsoleClient {
  constructor() {
    this.keepRunning = false;
    this.sensorList = null;
    this.sensor = null;
    this.updateInternal = null;
  }

  fetchFirstSensor() {
    console.log("var call getSensorList");
    this.sensorList = hsl.getSensorList();
    console.log("get senorList");

    console.log("Found " + this.sensorList.getSensorCount() + " sensors");
    if (this.sensorList.getSensorCount() > 0) {
      this.sensor = this.sensorList.getSensor(0);
      console.log("  Sensor ID: " + this.sensor.getDeviceFriendlyName());
    }
  }

  update() {
    hsl.update();

    if (hsl.hasSensorListChanged()) {
      console.log("sensor list changed");
      this.sensor = null;
      this.fetchFirstSensor();

      if (this.sensor != null) {
        if (this.sensor.hasCapability(hsl.Sensor.StreamFlags_ECGData))
          this.sensor.setDataStreamActive(hsl.Sensor.StreamFlags_ECGData, true)
        else if (this.sensor.hasCapability(hsl.Sensor.StreamFlags_PPGData))
          this.sensor.setDataStreamActive(hsl.Sensor.StreamFlags_PPGData, true)
        else if (this.sensor.hasCapability(hsl.Sensor.StreamFlags_HRData))
          this.sensor.setDataStreamActive(hsl.Sensor.StreamFlags_HRData, true)
      }
      else {
        console.log("No Sensors found. Waiting...");
      }
    }

    if (this.sensor != null) {
      var ppgBufferIter = this.sensor.getHeartPPGBuffer();

      while (ppgBufferIter.isValid()) {
        var ppgData = ppgBufferIter.getPPGData();
        var time = ppgData['timeInSeconds'];
        var samples = ppgData['ppgSamples'];

        samples.forEach(s => {
          console.log("[0:" + s['ppgValue0'] + ", 1:" + s['ppgValue1'] + ", 2:" + s['ppgValue2'] + "], amb:" + s['ambient'])
        });

        ppgBufferIter.next();
      }
    }
  }

  shutdown() {
    if (this.sensor != null) {
      console.log("Stopping sensor stream");
      this.sensor.stopAllStreams();
    }

    clearInterval(this.updateInternal);
  }

  startup() {
    console.log("HSL " + hsl.getVersionString());

    process.stdin.on('keypress', (str, key) => {
      if (key.ctrl && key.name === 'c') {
        process.exit(); // eslint-disable-line no-process-exit
      }
    });
  }

  run() {
    this.startup();
    var _this = this;
    this.updateInternal = setInterval(function () { _this.update(); }, 100);
  }
}

let hslClient = new HSLConsoleClient();
hslClient.run()