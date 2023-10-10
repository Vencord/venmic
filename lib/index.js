const binding = require("pkg-prebuilds")(
    require("path").resolve(__dirname, ".."),
    require("./options.js")
);

Object.keys(binding).forEach(function (key)
{
    module.exports[key] = binding[key];
});
