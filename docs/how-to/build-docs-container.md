# Containerized Documentation Build

This guide shows how to build zpmod documentation using Docker containers for consistent, reproducible documentation generation across
different environments.

## Quick Start

### Build Documentation in Container

```bash
# Standard documentation build
./scripts/docker-test.sh docs

# Clean rebuild (removes previous build first)
./scripts/docker-test.sh docs-clean
```

### Using Docker Compose Directly

```bash
# Change to docker directory
cd docker

# Standard build
docker-compose run --rm build-docs

# Clean rebuild
docker-compose run --rm build-docs-clean
```

## What Gets Built

The containerized documentation build generates:

- **HTML Documentation**: Complete API reference with modern responsive design
- **Source Code Browser**: Interactive source browsing with syntax highlighting
- **Dependency Diagrams**: Call graphs and include relationships via GraphViz
- **Cross-References**: Links between functions, files, and documentation
- **Search Index**: Client-side search functionality

## Output Location

Generated documentation is available at:

```
build-results/docs/html/index.html
```

You can open this directly in your browser:

```bash
# Open documentation in default browser (Linux)
xdg-open build-results/docs/html/index.html

# Open documentation in default browser (macOS)
open build-results/docs/html/index.html

# Or view the file path
echo "file://$(pwd)/build-results/docs/html/index.html"
```

## Container Features

### Documentation Environment

The `docker/docs.Dockerfile` provides:

- **Ubuntu 22.04** base with comprehensive documentation toolchain
- **Doxygen** for generating API documentation from code comments
- **GraphViz** for creating dependency and call relationship diagrams
- **LaTeX** support for mathematical formula rendering
- **Ghostscript** for PostScript/PDF processing
- **All dependencies** isolated and versioned consistently

### Build Process

The container automatically:

1. **Configures** CMake build system for documentation
2. **Generates** Doxygen configuration from templates
3. **Builds** HTML documentation with full cross-references
4. **Creates** visual diagrams showing code relationships
5. **Outputs** results to mounted volume for easy access

## Environment Variables

Customize the build with environment variables:

```bash
# Build type (Release/Debug)
ZPMOD_BUILD_TYPE=Release

# Enable verbose CMake output
CMAKE_VERBOSE_MAKEFILE=ON
```

## Integration with CI/CD

The containerized docs build integrates with:

- **GitHub Actions**: `.github/workflows/docs.yml` for automated documentation
- **Local Development**: Consistent environment matching CI
- **GitHub Pages**: Direct deployment of generated HTML documentation

## Troubleshooting

### Common Issues

**Docker build fails:**

```bash
# Clean Docker cache and rebuild
docker system prune -f
cd docker && docker-compose build --no-cache build-docs
```

**Documentation not generated:**

```bash
# Check for Doxygen errors in container logs
cd docker && docker-compose run --rm build-docs 2>&1 | grep -i error
```

**Output directory not found:**

```bash
# Ensure build-results directory exists
mkdir -p build-results/docs
```

### Container Debugging

Run interactive shell in docs container:

```bash
cd docker
docker-compose run --rm --entrypoint /bin/bash build-docs

# Inside container, manually run build steps:
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake --target docs --verbose
```

## Comparison with Local Build

| Method        | Advantages                                                | Requirements             |
| ------------- | --------------------------------------------------------- | ------------------------ |
| **Container** | Consistent environment, no local dependencies, matches CI | Docker                   |
| **Local**     | Faster iteration, direct file access                      | Doxygen, GraphViz, LaTeX |

### Local Build Alternative

For local development without Docker:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install doxygen graphviz texlive-latex-base

# Build using CMake helper script
./scripts/cmake.configure.zsh --docs

# Or build directly with CMake
cmake -S . -B build-cmake
cmake --build build-cmake --target docs
```

## Advanced Usage

### Custom Documentation Configuration

The container uses the same Doxygen configuration as local builds:

- **Primary config**: `cmake/Doxyfile.in` (CMake template)
- **Fallback config**: `docs/Doxyfile.in` (direct template)
- **Output customization**: Modify templates for different output formats

### Batch Documentation Operations

```bash
# Build docs as part of full test suite
./scripts/docker-test.sh all

# Build docs + run tests in sequence
./scripts/docker-test.sh docs && ./scripts/docker-test.sh smoke
```

## CI/CD Integration

The project uses GitHub Actions for automated documentation builds and deployment:

### Workflow Overview

- **Triggers**: Push to main/develop branches, manual dispatch
- **Environment**: GitHub-hosted Ubuntu runner with Docker
- **Build**: Uses same containerized process as local development
- **Deployment**: Automatic GitHub Pages publishing

### Key Features

- **Consistency**: Same Docker container for local and CI builds
- **Reliability**: No dependency installation issues or version conflicts
- **Speed**: Pre-built container with cached dependencies
- **Verification**: Automated output validation and statistics

### Workflow File

The CI/CD configuration is in `.github/workflows/docs.yml`:

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: docker-compose run --rm build-docs
      - uses: actions/upload-pages-artifact@v3
        with:
          path: build-results/docs/html
```

### Build Process

1. **Repository Checkout**: Source code downloaded with full history
2. **Environment Setup**: Build output directory initialized
3. **Containerized Build**: Documentation generated using Docker Compose
4. **Validation**: Output files verified and statistics collected
5. **Artifact Upload**: HTML documentation prepared for GitHub Pages
6. **Deployment**: Automatic publishing to project GitHub Pages site

### Monitoring

- **Build Status**: Visible in repository Actions tab
- **Deploy URL**: Available at `https://{owner}.github.io/{repo}/`
- **Build Logs**: Detailed output for debugging failures
- **Statistics**: File counts and size metrics in CI output

### Documentation Deployment

```bash
# Generate docs for GitHub Pages deployment
cd docker && docker-compose run --rm build-docs

# Upload to web server (example)
rsync -av build-results/docs/html/ user@server:/var/www/zpmod-docs/
```

## Related Documentation

- [Main documentation index](../index.md)
- [CMake build helper reference](../reference/install-cmake-helper.md)
- [Commenting guidelines](../explanation/commenting.md)
- [GitHub Actions workflows](../../.github/workflows/docs.yml)
