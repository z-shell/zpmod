# Release staging in container

This shows how to stage a release tree using the container so files are owned by your host user/group.

## Prerequisites

- Docker with compose plugin

## Quick usage

```sh
# From repo root
export USER_ID="$(id -u)"
export GROUP_ID="$(id -g)"
mkdir -p build-results/release
docker compose -f docker/docker-compose.yml run --rm build-release-clean
```

The staged tree will appear under `build-results/release/`.
