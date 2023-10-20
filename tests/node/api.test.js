const venmic = require("../../lib");
const assert = require("assert");

try
{
    const patchbay = new venmic.PatchBay();

    assert(Array.isArray(patchbay.list()));
    assert(Array.isArray(patchbay.list(["node.name"])));

    assert.throws(() => patchbay.list({}), /expected array/ig);
    assert.throws(() => patchbay.list([10]), /expected item to be string/ig);

    assert.throws(() => patchbay.link(10), /expected link object/ig);
    assert.throws(() => patchbay.link({ a: "A", b: "B", c: "C" }), /expected keys/ig);
    assert.throws(() => patchbay.link({ key: "node.name", value: "Firefox", mode: "gibberish" }), /expected mode/ig);

    assert.doesNotThrow(() => patchbay.link({ key: "node.name", value: "Firefox", mode: "include" }));
    assert.doesNotThrow(() => patchbay.unlink());
}
catch (error)
{
    console.warn("No PipeWire Server available");

    assert(!venmic.PatchBay.hasPipeWire());
    assert.throws(() => new venmic.PatchBay(), /(failed to create patchbay)|(not running pipewire)/ig);
}
