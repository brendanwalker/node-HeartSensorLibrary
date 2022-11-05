var hsl = require("./build/Debug/heartsensorlibrary.node");

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
        else if (sensor.hasCapability(hsl.Sensor.StreamFlags_HRData))
          sensor.setDataStreamActive(hsl.Sensor.StreamFlags_GSRData, true)
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

  publishSensorGSRStream(sensor) {
    var gsrBufferIter = sensor.getGalvanicSkinResponseBuffer();

    var gsrArray = [];
    while (gsrBufferIter.isValid()) {
      gsrArray.push(gsrBufferIter.getGSRData());
      gsrBufferIter.next();
    }

    if (gsrArray.length > 0) {
      sensor.flushGalvanicSkinResponseBuffer();

      publishData({ id: sensor.getSensorID(), type: "gsr", stream: gsrArray });
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
      _this.publishSensorGSRStream(sensor);
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

export { HSLSensorClient };