const venmic = require("../../lib");
const assert = require("assert");

assert(typeof venmic["list"] === "function");
assert(typeof venmic["unlink"] === "function");

assert.throws(() => venmic.link(10), /expected string/ig);
assert.throws(() => venmic.link("", ""), /one argument/ig);

assert.throws(() => venmic.link("Firefox"), /failed to create audio instance/ig);
