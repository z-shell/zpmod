# Minimal zpmod test environment
# Simulates CI environment with basic zsh installation (no development headers)
FROM ubuntu:22.04

# Install minimal dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    zsh \
    git \
    libncurses-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . .

# Clean any existing build directory to avoid cache conflicts
RUN rm -rf build-cmake

# Configure build
RUN cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release

# Build (should exclude source.c in this environment)
RUN cmake --build build-cmake -j 2

# Stage module
RUN cmake --build build-cmake --target stage

# Set default command to run tests
CMD ["ctest", "--test-dir", "build-cmake", "--output-on-failure", "-j", "2"]
