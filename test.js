const virtmic = require(".");

console.log(virtmic.getTargets())

virtmic.start("[All Desktop Audio]")
virtmic.start("[None]")
virtmic.stop();