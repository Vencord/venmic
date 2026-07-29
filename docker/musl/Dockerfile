# This Dockerfile describes the base image used for compiling the (glibc) node addon on different CPU-Architectures

FROM alpine:3

# Add scripts

ADD build.sh .
ADD dependencies.sh .

# Dependencies

RUN ./dependencies.sh
