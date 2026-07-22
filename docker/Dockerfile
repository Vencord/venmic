# This Dockerfile describes the base image used for compiling the node addon on different CPU-Architectures

FROM quay.io/fedora/fedora-minimal:44 

# Dependencies

RUN microdnf install -y gcc gcc-c++ make cmake nodejs npm git curl ninja-build 
RUN microdnf install -y pipewire-devel pipewire pipewire-pulseaudio pulseaudio-libs-devel

RUN npm install --global pnpm
RUN microdnf clean all

# Add build script

ADD build.sh .
