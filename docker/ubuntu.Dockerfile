#-----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Project helper documentation.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
#---------------------------------------------------------------------------#

# syntax=docker/dockerfile:1
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends     clang lld lldb cmake ninja-build git curl ca-certificates pkg-config     build-essential zip unzip &&     rm -rf /var/lib/apt/lists/*
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID builder && useradd -m -u $UID -g $GID builder
USER builder
WORKDIR /workspace
CMD ["/bin/bash", "-lc", "echo 'Mount repo at /workspace then: cmake -S . -B build -G Ninja && cmake --build build -j'"]
