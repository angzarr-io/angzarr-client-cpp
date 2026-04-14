# C++ client library commands
#
# Container Overlay Pattern:
# --------------------------
# This justfile uses an overlay pattern for container execution:
#
# 1. `justfile` (this file) - runs on the host, delegates to container
# 2. `justfile.container` - mounted over this file inside the container
#
# When running outside a devcontainer:
#   - Uses pre-built angzarr-cpp image from ghcr.io/angzarr
#   - Podman mounts justfile.container as /workspace/justfile
#
# When running inside a devcontainer (DEVCONTAINER=true):
#   - Commands execute directly via `just <target>`
#   - No container nesting

set shell := ["bash", "-c"]

ROOT := `git rev-parse --show-toplevel`
IMAGE := "ghcr.io/angzarr-io/angzarr-cpp:latest"

# Run just target in container (or directly if already in devcontainer)
[private]
_container +ARGS:
    #!/usr/bin/env bash
    if [ "${DEVCONTAINER:-}" = "true" ]; then
        just {{ARGS}}
    else
        podman run --rm --network=host \
            -v "{{ROOT}}:/workspace:Z" \
            -v "{{ROOT}}/justfile.container:/workspace/justfile:ro" \
            -w /workspace \
            -e DEVCONTAINER=true \
            {{IMAGE}} just {{ARGS}}
    fi

default:
    @just --list

proto:
    just _container proto

configure:
    just _container configure

build:
    just _container build

test:
    just _container test

# Start gRPC test server for unified Rust harness testing
serve:
    just _container serve

# Create release archive
archive VERSION:
    just _container archive {{VERSION}}

clean:
    rm -rf "{{ROOT}}/build"

# Check formatting
fmt:
    just _container fmt

# Auto-format code
fmt-fix:
    just _container fmt-fix
