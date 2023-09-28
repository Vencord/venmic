# Vencord Virtmic

A simple command line program to allow electron apps to stream with audio on Linux via pipewire

## Usage

Grab the [prebuild](/prebuilt/) or [build from source](#compiling)

#### List available targets (apps that are playing audio)

This will print a list of names, newline separated

```
./vencord-virtmic --list-targets
```

#### Create a virtual mic for a target

This will create a virtual mic called `vencord-virtmic` you can use

```
./vencord-virtmic Firefox
```

## Compiling

I did this on fedora, but it should work similarly on other distros.

- Install the build essentials (make, cmake, g++, ...)
- Install pipewire-devel, expected-devel,

```sh
git clone https://github.com/Vencord/linux-virtmic --recurse-submodules
mkdir build
cd build
cmake ..
make
```

You should now find the addon at build/vencord-virtmic

## Acknowledgements

This library is heavily based on the amazing work by the https://github.com/Soundux/rohrkabel/ library
