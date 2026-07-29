#!/bin/sh

microdnf install -y git tar nodejs npm curl libatomic
microdnf install -y make automake gcc gcc-c++ cmake ninja-build
microdnf install -y pipewire-devel pipewire-libs pulseaudio-libs-devel pipewire-pulseaudio

# Setup pnpm

npm install --global pnpm
