const venmic = require("../../lib");
const assert = require("assert");

let patchbay = null;

try
{
    patchbay = new venmic.PatchBay();
}
catch (error)
{
    console.warn("No PipeWire Server available");

    assert(!venmic.PatchBay.hasPipeWire());
    assert.throws(() => new venmic.PatchBay(), /(failed to create patchbay)|(not running pipewire)/ig);

    process.exit(0);
}

assert(Array.isArray(patchbay.list()));
assert(Array.isArray(patchbay.list(["node.name"])));

assert.throws(() => patchbay.list({}), /expected list of strings/ig);
assert.throws(() => patchbay.list([10]), /expected list of strings/ig);

assert.throws(() => patchbay.link(10), /expected link object/ig);

assert.throws(() => patchbay.link({ a: "A", b: "B", c: "C" }), /expected keys/ig);
assert.throws(() => patchbay.link({ key: "node.name", value: "Firefox", mode: "gibberish" }), /expected keys/ig);

assert.throws(() => patchbay.link({ props: [{ a: 0, b: 0 }], mode: "gibberish" }), /key-value/ig);
assert.throws(() => patchbay.link({ props: [{ key: "", value: "" }], mode: "gibberish" }), /expected mode/ig);

assert.doesNotThrow(() => patchbay.link({ props: [{ key: "node.name", value: "Firefox" }], mode: "include" }));
assert.doesNotThrow(() => patchbay.unlink());
