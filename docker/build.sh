#!/bin/bash

# Ensure pnpm is loaded
source /root/.bashrc

# Let's get to work!
cd /work

# Export Threads for Make and Ninja
export MAKEFLAGS=-j$(nproc)
export PARALLEL_LEVEL=$(nproc)

pnpm install --ignore-scripts && pnpm run install:static
