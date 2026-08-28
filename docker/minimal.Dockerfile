# Minimal zpmod test environment
# Simulates CI environment with basic zsh installation (no development headers)
FROM ubuntu:22.04

# Install minimal dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    autoconf \
    cmake \
    zsh \
    git \
    ca-certificates \
    libncurses-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Fail closed if the distribution package stops providing the documented floor.
ARG ZPMOD_MINIMUM_ZSH=5.8.1
RUN actual_version="$(zsh -fc 'print -r -- "$ZSH_VERSION"')" && \
    if [ "$actual_version" != "$ZPMOD_MINIMUM_ZSH" ]; then \
      printf 'Expected Zsh %s, found %s\n' \
        "$ZPMOD_MINIMUM_ZSH" "$actual_version" >&2; \
      exit 1; \
    fi && \
    printf 'Using minimum supported Zsh %s\n' "$actual_version"

# Create non-root user for running tests so permission checks (EACCES) behave as expected
RUN useradd -m -u 1000 -U zp

# Set working directory
WORKDIR /workspace

# Copy project files with ownership to non-root user
COPY --chown=zp:zp . .
# Ensure workspace dir itself is writable by non-root user
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
            -DZSH_EXECUTABLE=/usr/bin/zsh && \
    cmake --build build-cmake -j 2 && \
    cmake --build build-cmake --target stage

# Set default command to run tests
CMD ["ctest", "--test-dir", "build-cmake", "--output-on-failure", "-j", "2"]
