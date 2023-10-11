const binding = require("pkg-prebuilds")(
    require("path").resolve(__dirname, ".."),
    require("./options.js")
);

module.exports = binding;
