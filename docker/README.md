# Docker-based Testing for zpmod

This directory contains Docker configurations for testing zpmod in various environments to ensure consistent CI/CD behavior and validate
conditional compilation logic.

## Test Scenarios

### 1. Minimal Environment (`minimal.Dockerfile`)

- **Purpose**: Simulates CI environment with basic zsh installation
- **Features**: Tests core functionality, validates source.c exclusion
- **Use case**: Ensures module works in constrained environments

### 2. Full Development Environment (`full-dev.Dockerfile`)

- **Purpose**: Complete zsh development environment with all headers
- **Features**: Tests all functionality including source study features
- **Use case**: Validates full feature set in development environments

### 3. Multi-version Testing (`multi-version.Dockerfile`)

- **Purpose**: Tests against different zsh versions
- **Features**: Compatibility validation across zsh releases
- **Use case**: Ensures forward/backward compatibility

## Usage

```bash
# Build all test environments
docker-compose build

# Run tests in minimal environment (should exclude source.c)
docker-compose run test-minimal

# Run tests in full development environment (should include source.c)
docker-compose run test-full-dev

# Run all test scenarios
docker-compose up --abort-on-container-exit

# Local development testing
./scripts/docker-test.sh minimal
./scripts/docker-test.sh full-dev
```

## Benefits

- **Reproducible CI failures**: Same environment locally and in CI
- **Predictable dependencies**: Controlled zsh installation scenarios
- **Conditional compilation validation**: Test both inclusion/exclusion of source.c
- **Environment isolation**: No host dependency conflicts
- **Multi-scenario testing**: Validate different deployment environments
