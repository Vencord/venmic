const binding = require("pkg-prebuilds")(
    require("path").resolve(__dirname, ".."),
    require("./options")
);

module.exports = binding;
