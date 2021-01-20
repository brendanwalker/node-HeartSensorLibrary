var hslModule = require("./build/Release/heartsensorlibrary.node");
module.exports = hslModule;

console.log("Initializing HSL");
hslModule.initialize();

console.log("Updateing HSL");
hslModule.update();

console.log("shutdown HSL");
hslModule.shutdown();