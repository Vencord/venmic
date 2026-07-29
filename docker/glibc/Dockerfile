# This Dockerfile describes the base image used for compiling the (glibc) node addon on different CPU-Architectures

FROM quay.io/fedora/fedora-minimal:44

# Add scripts

ADD build.sh .
ADD dependencies.sh .

# Dependencies

RUN ./dependencies.sh
RUN microdnf clean all
