const venmic = require("../../lib");
const assert = require("assert");

assert(Array.isArray(venmic.list()));

assert.throws(() => venmic.link(10));
assert.throws(() => venmic.link("", ""));


assert.doesNotThrow(() => venmic.link("Firefox"));
assert.doesNotThrow(() => venmic.unlink());
