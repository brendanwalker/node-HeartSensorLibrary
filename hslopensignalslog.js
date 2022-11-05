hsl = require("./build/Debug/heartsensorlibrary.node");
fs = require('fs');
util = require('util');
dateFormat = require('dateformat');

class HSLOpenSignalsLogger {
  constructor(sensorClient, sensorType, filename) {
    this.hslClient = sensorClient;
    this.hslSensorType = sensorType;
    this.logFilename = filename;
    this.logStream = null;
    this.sensorId = -1;
    this.sampleIndex = 0;
  }

  open() {
    this.logStream = fs.createWriteStream(this.logFilename, { flags: 'a' });

    this.hslClient.addListener(this, this.handleSensorData);
    this.hslClient.start();
  }

  close() {
    if (this.logStream != null) {
      this.logStream.end();
      this.logStream = null;
    }
  }

  handleSensorData(data) {
    const dataType = data["type"];
    if (dataType != this.hslSensorType)
      return;

    const dataSensorId = data["id"];
    if (this.sensorId == -1) {
      this.sensorId = dataSensorId;
      writeHeader();
    } else if (this.sensorId != dataSensorId) {
      return;
    }

    const sensorStreamData = data['stream'];
    sensorStreamData.forEach(function (streamData) {
      writeStreamData(streamData);
    });
  }

  writeHeader() {
    let sensorList = hsl.getSensorList();
    let sensor = sensorList.getSensor(this.sensorId);
    let now = new Date();

    info = {}
    info['sensor'] = ['RAW'];
    info['device name'] = sensor.getDeviceFriendlyName();
    info['column'] = ['nSeq', 'A3'];
    info['sync interval'] = 2;
    info['time'] = dateFormat(now, "h:MM:ss TT");
    info['comments'] = "";
    info['device connection'] = sensor.getDevicePath();
    info['channels'] = [1];
    info['date'] = dateFormat(now, "mmmm dS, yyyy");
    info['mode'] = 0;
    info['digital IO'] = [0];
    info['firmware version'] = sensor.getFirmwareRevisionString();
    info['device'] = sensor.getManufacturerNameString();
    info["position"] = 0;
    info['sampling rate'] = 100; // sensor.getDataStreamSampleRate(this.dataStreamType)
    info['label'] = ['A3'];
    info['resolution'] = [1];
    info['special'] = [{}];

    device = {};
    device[sensor.getSerialNumberString()] = info;

    this.logStream.write('# OpenSignals Text File Format\r\n');
    this.logStream.write('# ' + JSON.stringify(device));
    this.logStream.write('# EndOfHeader\r\n');
  }

  writeStreamData(streamData) {
    this.sampleIndex++;
  }
}

class HSLOpenSignalsGSRLogger extends HSLOpenSignalsLogger {
  constructor(sensorClient, filename) {
    super(sensorClient, "GSR", filename)
  }

  writeStreamData(streamData) {
    super.writeStreamData(streamData);

    const wrappedSampleIndex = this.sampleIndex % 16;
    const gsrSample = streamData['gsrValue'];

    this.logStream.write(util.format('%d\t:%d\r\n', wrappedSampleIndex, gsrSample));
  }
}

class HSLOpenSignalsECGLogger extends HSLOpenSignalsLogger {
  constructor(sensorClient, filename) {
    super(sensorClient, "ECG", filename)
  }

  writeStreamData(streamData) {
    const ecgValues = streamData['ecgValues'];
    ecgValues.forEach(function (ecgValue) {
      super.writeStreamData(streamData);
      const wrappedSampleIndex = this.sampleIndex % 16;
      this.logStream.write(util.format('%d\t:%d\r\n', wrappedSampleIndex, ecgValue));
    });
  }
}

export { HSLOpenSignalsGSRLogger };