# Full development zpmod test environment
# Includes complete zsh source and development headers
FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    groff-base \
    util-linux \
    autoconf \
    automake \
    libtool \
    texinfo \
    yodl \
    ncurses-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Build zsh from source to get full development environment
RUN git clone --depth 1 --branch zsh-5.9 https://github.com/zsh-users/zsh.git /tmp/zsh
WORKDIR /tmp/zsh
RUN ./Util/preconfig && \
    ./configure --enable-modules --enable-dynamic --with-tcsetpgrp && \
    make -j"$(nproc)" && \
    make install || make install.bin install.modules install.fns && \
    rm -rf /tmp/zsh

# Set working directory
WORKDIR /workspace

# Create non-root user for running tests
RUN useradd -m -u 1000 -U zp

# Copy project files with ownership to non-root user
COPY --chown=zp:zp . .
RUN chown -R zp:zp /workspace

# Switch to non-root user
USER zp

# Fresh submodule checkouts do not contain Zsh's ignored generated headers.
# Build the vendored source to generate the headers before configuring zpmod.
RUN cd vendor/zsh && \
        ./Util/preconfig && \
        ./configure && \
        make -j 2 && \
        test -s config.h && \
        test -s Src/zsh.mdh

RUN rm -rf build-cmake && \
        cmake -S . -B build-cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_TESTING=ON \
            -DZSH_EXECUTABLE=/usr/local/bin/zsh && \
    cmake --build build-cmake -j 2 && \
    cmake --build build-cmake --target stage

# Set default command to run tests
CMD ["ctest", "--test-dir", "build-cmake", "--output-on-failure", "-j", "2"]
