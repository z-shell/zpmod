# Full development zpmod test environment
# Includes complete zsh source and development headers
FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    autoconf \
    automake \
    libtool \
    texinfo \
    ncurses-dev \
    && rm -rf /var/lib/apt/lists/*

# Build zsh from source to get full development environment
RUN git clone --depth 1 --branch zsh-5.9 https://github.com/zsh-users/zsh.git /tmp/zsh && \
    :
WORKDIR /tmp/zsh
RUN ./Util/preconfig && \
    ./configure --enable-modules --enable-dynamic --with-tcsetpgrp && \
    make -j"$(nproc)" && \
    make install && \
    cd / && rm -rf /tmp/zsh

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . .

RUN rm -rf build-cmake && \
    cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build-cmake -j 2 && \
    cmake --build build-cmake --target stage

# Set default command to run tests
CMD ["ctest", "--test-dir", "build-cmake", "--output-on-failure", "-j", "2"]
