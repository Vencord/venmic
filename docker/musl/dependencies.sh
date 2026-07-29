#!/bin/bash

apk add curl git tar nodejs npm libatomic
apk add gcc g++ cmake ninja-build
apk add pipewire pipewire-dev pipewire-libs pipewire-pulse pulseaudio-dev

# Setup pnpm

npm install --global pnpm
