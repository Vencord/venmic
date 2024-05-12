#!/bin/bash

source /root/.bashrc

cd /work
pnpm install --ignore-scripts && pnpm run install
