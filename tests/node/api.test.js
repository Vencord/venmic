const venmic = require("../../lib");
const assert = require("assert");

assert(typeof venmic["list"] === "function");
assert(typeof venmic["unlink"] === "function");

assert.throws(() => venmic.link(10));
assert.throws(() => venmic.link("", ""));

// Should throw because pipewire is not available.
assert.throws(() => venmic.link("Firefox"));
