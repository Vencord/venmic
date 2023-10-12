const venmic = require("../../lib");
const assert = require("assert");

try
{
    const audio = new venmic.audio();

    assert(Array.isArray(audio.list()));

    assert.throws(() => audio.link(10), /expected string/ig);
    assert.throws(() => audio.link("", ""), /one argument/ig);

    assert.doesNotThrow(() => audio.link("Firefox"));
    assert.doesNotThrow(() => audio.unlink());
}
catch (error)
{
    console.log("No PipeWire Server available");
    assert.throws(() => new venmic.audio(), /failed to create audio instance/ig);
}
