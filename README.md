This is not finished yet and not usable in its current state


# Vencord Virtmic

A node native addon to allow electron apps to stream with audio on Linux, via pipewire

## Usage

Grab [the prebuilt addon](prebuilt/) or, build it from source as described in [the build instructions](#Compiling)

The addon offers two methods:

- `getTargets()` returns an array containing the available audio targets (apps that transmit audio)
- `start(target)` creates a virtual mic for the specified target. Target can be:
  - `"[All Desktop Audio]"` or `[None]`
  - A target listed in the return value of `getTargets`

## Compiling

I did this on fedora, but it should work similarly on other distros.

- Install the build essentials (make, cmake, g++)
- Install nodejs-devel, pipewire-devel, expected-devel,

```sh
git clone [repo url] --recurse-submodules
cd submodules/rohrkabel
mkdir build
cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
cd ../../..
pnpm i
pnpm build
```

You should now find the addon at build/Release/vencordvirtmic.node

## Acknowledgements

This library is heavily based on the amazing work by maltejur and other contributors to https://github.com/maltejur/discord-screenaudio
