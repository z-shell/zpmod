# Documentation build environment for zpmod
# Provides consistent documentation generation with Doxygen, GraphViz, and LaTeX support
FROM ubuntu:26.04@sha256:f144425ff09be612d6d9ad965196e9cdc23dae1f42110a8a11a3e9a8198759f7

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install documentation toolchain
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Core build tools
    build-essential \
    cmake \
    git \
    libncurses-dev \
    # Documentation generation
    doxygen \
    graphviz \
    # LaTeX support for formula rendering (optional but recommended)
    texlive-latex-base \
    texlive-latex-extra \
    texlive-fonts-recommended \
    # PostScript/PDF processing
    ghostscript \
    # Additional utilities
    curl \
    wget \
    sudo \
    gosu \
    && rm -rf /var/lib/apt/lists/*

# Verify installation
RUN doxygen --version && \
    dot -V && \
    cmake --version && \
    pdflatex --version

# Set working directory
WORKDIR /workspace

# Create output directory structure
RUN mkdir -p build-cmake/docs

# Add entrypoint script to handle user/group ownership
COPY docker/entrypoint-docs.sh /usr/local/bin/entrypoint-docs.sh
RUN chmod +x /usr/local/bin/entrypoint-docs.sh

# Use entrypoint to handle user setup and ownership
ENTRYPOINT ["/usr/local/bin/entrypoint-docs.sh"]

# Default command: configure and build documentation
CMD ["sh", "-c", "cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release && cmake --build build-cmake --target docs"]
