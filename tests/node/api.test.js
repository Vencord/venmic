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
    assert.throws(() => new venmic.PatchBay(), /failed to create patchbay/ig);

    process.exit(0);
}

assert(Array.isArray(patchbay.list()));
assert(Array.isArray(patchbay.list(["node.name"])));

assert.throws(() => patchbay.list({}), /expected list of strings/ig);
assert.throws(() => patchbay.list([10]), /expected list of strings/ig);

assert.throws(() => patchbay.link(10), /expected link object/ig);

assert.throws(() => patchbay.link({ }), /'include' or 'exclude'/ig);
assert.throws(() => patchbay.link({ a: "A", b: "B", c: "C" }), /'include' or 'exclude'/ig);
assert.throws(() => patchbay.link({ "node.name": "Firefox", mode: "gibberish" }), /'include' or 'exclude'/ig);

assert.throws(() => patchbay.link({ include: "Firefox" }), /key-value/ig);
assert.throws(() => patchbay.link({ include: {} }), /key-value/ig);

assert.doesNotThrow(() => patchbay.link({ include: [{ "node.name": "Firefox" }] }));
assert.doesNotThrow(() => patchbay.link({ exclude: [{ "node.name": "Firefox" }] }));

assert.doesNotThrow(() => patchbay.link({ include: [{ "node.name": "Firefox" }], exclude: [{ "object.id": "100" }] }));

assert.doesNotThrow(() => patchbay.link({ exclude: [{ "node.name": "Firefox" }], ignore_devices: true }));
assert.doesNotThrow(() => patchbay.link({ exclude: [{ "node.name": "Firefox" }], ignore_devices: true, only_default_speakers: true }));

assert.doesNotThrow(() => patchbay.unlink());
