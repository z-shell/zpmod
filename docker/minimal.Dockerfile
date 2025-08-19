# Minimal zpmod test environment
# Simulates CI environment with basic zsh installation (no development headers)
FROM ubuntu:22.04

# Install minimal dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    zsh \
    git \
    ca-certificates \
    libncurses-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

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

RUN rm -rf build-cmake && \
        cmake -S . -B build-cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_TESTING=ON \
            -DZSH_EXECUTABLE=/usr/bin/zsh && \
    cmake --build build-cmake -j 2 && \
    cmake --build build-cmake --target stage

# Set default command to run tests
CMD ["ctest", "--test-dir", "build-cmake", "--output-on-failure", "-j", "2"]
