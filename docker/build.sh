#!/bin/bash

if [ -z "${VENMIC_NOT_DOCKER}" ]; then
    source /root/.bashrc
    cd /work
fi

export MAKEFLAGS=-j$(nproc)
export PARALLEL_LEVEL=$(nproc)

pnpm install --ignore-scripts && pnpm run install
