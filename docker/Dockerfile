# This Dockerfile describes the base image used for compiling the node addon on different CPU-Architectures

FROM fedora:38

# Build dependencies

RUN dnf install -y make automake gcc gcc-c++ kernel-devel cmake git nodejs libstdc++-static
RUN dnf install -y pipewire-devel pipewire-libs pulseaudio-libs-devel pipewire-pulseaudio

# PNPM

RUN curl -fsSL https://get.pnpm.io/install.sh | sh -

# Add build script

ADD build.sh .
