# Documentation build environment for zpmod
# Provides consistent documentation generation with Doxygen, GraphViz, and LaTeX support
FROM ubuntu:22.04@sha256:0a9887931b0e38af2e3487ed39e29436b0535c4f3f52e57745baaf79d1898956

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
