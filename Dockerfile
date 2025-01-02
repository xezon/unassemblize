FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    ccache \
    clang \
    clang-format \
    cmake \
    doxygen \
    git \
    graphviz \
    make \
    pkg-config \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    mesa-common-dev \
    libglx-dev \
    libx11-dev \
    xorg-dev \
    && rm -rf /var/lib/apt/lists/*

# Set Clang as the default compiler
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

# Set up working directory
WORKDIR /app

# Create a build script to handle the LIEF color diagnostics fix
RUN echo '#!/bin/bash\n\
mkdir -p build && cd build\n\
cmake ..\n\
sed -i "s/-fcolor-diagnostics//" _deps/lief-build/CMakeFiles/LIB_LIEF.dir/flags.make\n\
make' > /app/build.sh && chmod +x /app/build.sh

# Set the default command to run an interactive shell
CMD ["/bin/bash"]
