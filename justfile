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
#   - Uses pre-built angzarr-cpp image from ghcr.io/angzarr-io
#   - Docker mounts justfile.container as /workspace/justfile
#
# When running inside a devcontainer (DEVCONTAINER=true):
#   - Commands execute directly via `just <target>`
#   - No container nesting

set shell := ["bash", "-c"]

# Reusable submodule-protection recipes (install-submodule-hooks,
# check-submodules-clean). Source of truth: angzarr-project/submodule.just.
import? 'angzarr-project/submodule.just'

ROOT := `git rev-parse --show-toplevel`
# Container image is digest-pinned (project_supply_chain_digest_pinning).
# `:latest` is kept on the LHS of `@` so `docker pull` still resolves the
# float when an admin wants to spot-check what `latest` currently points at,
# but the actual run uses the immutable sha256. Bumps to a newer image must
# go through the per-repo bumper PR alongside the other languages'.
IMAGE := "ghcr.io/angzarr-io/angzarr-cpp:latest@sha256:3c66dd0ffc7d2dd727c355d1b22c2741abce4d570baea4517c77fd5d40d97bfe"

# Run just target in container (or directly if already in devcontainer).
# Rootless docker: -u 0:0 maps to host user via subuid; writes to the
# bind-mount land owned by the host user. Rootful: direct uid match.
# See feedback_docker_rootless.
[private]
_container +ARGS:
    #!/usr/bin/env bash
    if [ "${DEVCONTAINER:-}" = "true" ]; then
        just {{ARGS}}
    else
        if docker info --format '{{{{.SecurityOptions}}}}' 2>/dev/null | grep -q rootless; then
            USER_FLAG="-u 0:0"
        else
            USER_FLAG="-u $(id -u):$(id -g)"
        fi
        docker run --rm --network=host \
            $USER_FLAG \
            -v "{{ROOT}}:/workspace:Z" \
            -v "{{ROOT}}/justfile.container:/workspace/justfile:ro" \
            -w /workspace \
            -e DEVCONTAINER=true \
            {{IMAGE}} just {{ARGS}}
    fi

# Run a mutation-testing target with the workspace mounted READ-ONLY.
#
# WHY:
#   Mull mutates compiled LLVM IR — it does NOT modify .cpp/.hpp source.
#   The host-leak risk is therefore for build/ artifacts (instrumented
#   binaries, mull state) rather than source. We still isolate via an
#   overlay copy so:
#     1. Crashed runs cannot leave stray build-mull/ trees on the host.
#     2. The pattern matches the other angzarr-*-{lang} repos for
#        operational consistency (one mental model).
#   Source mounts read-only at /src; build happens in /work inside the
#   container's writable overlay; `--rm` wipes it on exit.
#
# WHAT TOUCHES THE HOST:
#   - {{ROOT}}/.mutants-cache/ccache — ccache compiler cache, persisted
#     across runs so mutation rebuilds stay fast. NEVER holds source.
#   - {{ROOT}}/mull-reports/      — mull-runner reports (HTML, json,
#     mull.sqlite) copied out on exit.
#
# WHAT NEVER TOUCHES THE HOST:
#   - The build-mull/ tree (lives in /work, container overlay, --rm wipes).
#   - Mutated LLVM IR / intermediate mull working state.
[private]
_container-ephemeral +ARGS:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ "${DEVCONTAINER:-}" = "true" ]; then
        # Already inside a devcontainer — that container IS the ephemeral
        # boundary. Run directly; the outer `--rm` keeps the contract.
        just {{ARGS}}
        exit 0
    fi
    mkdir -p "{{ROOT}}/mull-reports" \
             "{{ROOT}}/.mutants-cache/ccache"
    if docker info --format '{{{{.SecurityOptions}}}}' 2>/dev/null | grep -q rootless; then
        USER_FLAG="-u 0:0"
    else
        USER_FLAG="-u $(id -u):$(id -g)"
    fi
    docker run --rm --network=host \
        $USER_FLAG \
        -v "{{ROOT}}:/src:ro,Z" \
        -v "{{ROOT}}/mull-reports:/out:Z" \
        -v "{{ROOT}}/.mutants-cache/ccache:/ccache:Z" \
        -v "{{ROOT}}/justfile.container:/etc/angzarr-justfile:ro" \
        -e DEVCONTAINER=true \
        -e CCACHE_DIR=/ccache \
        -e MUTANTS_EPHEMERAL=1 \
        -w /work \
        {{IMAGE}} bash -eu -o pipefail -c '
            echo "[ephemeral] copying /src -> /work (container overlay)"
            mkdir -p /work
            # tar|tar: excludes mirror what we never want to copy —
            # prior build trees, the ephemeral cache, and prior reports.
            tar -C /src \
                --exclude=./build \
                --exclude=./build-mull \
                --exclude=./.mutants-cache \
                --exclude=./mull-reports \
                -cf - . \
                | tar -C /work -xf -
            # Mount the container-side justfile into the copy so `just`
            # finds it (the original /src is read-only, but /work is rw).
            cp /etc/angzarr-justfile /work/justfile
            cd /work
            just {{ARGS}}
            # Persist mull artifacts back to host for inspection.
            shopt -s nullglob
            for art in /work/build-mull/mull.sqlite \
                       /work/build-mull/**/mull.sqlite \
                       /work/mull.sqlite \
                       /work/*.html /work/*.json; do
                [ -e "$art" ] || continue
                cp "$art" /out/ 2>/dev/null || true
            done
            # Mull Elements reporter writes a directory tree.
            if [ -d /work/build-mull/mull-elements ]; then
                cp -r /work/build-mull/mull-elements /out/ 2>/dev/null || true
            fi
            echo "[ephemeral] mull artifacts (if any) copied to host mull-reports/"
        '

default:
    @just --list

# =============================================================================
# Proto generation — cross-language model (project_proto_generation_model)
# =============================================================================
# `.proto` sources live in the angzarr-project submodule. Generated C++
# bindings are NEVER committed (see .gitignore — both build/ and the
# proto/**/*.pb.{cc,h} pattern are excluded). They are regenerated:
#   1. on `post-checkout` / `post-merge` via lefthook (covers fresh clones,
#      branch switches, submodule bumps)
#   2. transparently as a recipe dependency of `build`, `test`, `fmt`, etc.
#      The recipe is idempotent — mtime guard skips when bindings are newer
#      than the newest .proto source.
#
# Runs in the same devcontainer image used for build/test/mutation so the
# protoc + grpc_cpp_plugin toolchain is fixed (no host fallback).
# Rootless docker requires `-u 0:0` per feedback_docker_rootless.
#
# Build-tool integration (CMake's `add_custom_command(OUTPUT … COMMAND
# protobuf::protoc …)`) is the EXECUTOR but NOT the trigger: this recipe
# explicitly invokes `cmake --build build --target angzarr-client` which
# resolves the proto .pb.{cc,h} dependencies via CMake's custom-command
# rules. Plain `cmake --build build` after generation is a no-op for the
# proto step (mtime-idempotent). Keeping the regen orchestration in `just`
# matches the 6-lang ecosystem pattern.

PROTO_SRC_DIR := ROOT + "/angzarr-project/proto"
PROTO_OUT_DIR := ROOT + "/build/generated"

# Public entry point. Idempotent: returns immediately if bindings are
# fresher than the newest .proto source.
generate-proto:
    #!/usr/bin/env bash
    set -euo pipefail
    src_dir="{{PROTO_SRC_DIR}}"
    out_dir="{{PROTO_OUT_DIR}}"
    if [ ! -d "$src_dir" ]; then
        echo "[generate-proto] $src_dir missing — is the angzarr-project submodule initialized?" >&2
        exit 1
    fi
    # Staleness check: regenerate if any .proto file is newer than the
    # OLDEST generated binding, or if no bindings exist yet.
    # Catches "submodule bumped" and "fresh clone" — the hot paths driving
    # the lefthook trigger. Does NOT catch manual deletion of one binding
    # while others remain fresh; use `just generate-proto-force` for that.
    #
    # OLDEST (matches Java/Python/Rust/C#) — the CMake build dir holds the
    # generated tree; `cmake -B build` does not wipe it but `just clean`
    # does, so no orphan-stale leftovers persist across regen runs.
    newest_proto=$(find "$src_dir" -name '*.proto' -printf '%T@\n' 2>/dev/null \
                    | sort -n | tail -1)
    # Guard the find for out_dir — on clean state build/ does not yet
    # exist, and `find $missing` exits non-zero which trips pipefail.
    if [ -d "$out_dir" ]; then
        oldest_pb=$(find "$out_dir" \( -name '*.pb.h' -o -name '*.pb.cc' \) \
                        -printf '%T@\n' 2>/dev/null | sort -n | head -1)
    else
        oldest_pb=""
    fi
    if [ -n "$newest_proto" ] && [ -n "$oldest_pb" ] \
        && awk -v p="$newest_proto" -v b="$oldest_pb" 'BEGIN{exit !(b>p)}'; then
        echo "[generate-proto] bindings up-to-date, skipping (use 'just generate-proto-force' to override)"
        exit 0
    fi
    just generate-proto-force

# Always regenerate, ignoring mtimes. Invoked by `generate-proto` when stale
# and exposed directly for users who want to force a rebuild.
generate-proto-force:
    #!/usr/bin/env bash
    set -euo pipefail
    if [ "${DEVCONTAINER:-}" = "true" ]; then
        # Inside the devcontainer image already — run directly.
        just --justfile "{{ROOT}}/justfile.container" generate-proto-force
        exit 0
    fi
    # Rootless docker: -u 0:0 maps to host user via subuid; writes to the
    # bind-mount land owned by the host user. Rootful: direct uid match.
    # See feedback_docker_rootless.
    if docker info --format '{{{{.SecurityOptions}}}}' 2>/dev/null | grep -q rootless; then
        USER_FLAG="-u 0:0"
    else
        USER_FLAG="-u $(id -u):$(id -g)"
    fi
    docker run --rm --network=host \
        $USER_FLAG \
        -v "{{ROOT}}:/workspace:Z" \
        -v "{{ROOT}}/justfile.container:/workspace/justfile:ro" \
        -w /workspace \
        -e DEVCONTAINER=true \
        {{IMAGE}} just generate-proto-force

# Legacy alias — kept so existing recipe-deps and muscle memory keep working.
proto: generate-proto

configure: generate-proto
    just _container configure

build: generate-proto
    just _container build

test: generate-proto
    just _container test

# Start gRPC test server for unified Rust harness testing
serve: generate-proto
    just _container serve

# === Mutation Testing ===
# All mull-runner runs go through `_container-ephemeral` so the build-mull/
# tree lives in the container's writable overlay layer and is destroyed
# with `--rm`. Mull does NOT mutate source files (it works on LLVM IR of
# compiled objects), but routing through the ephemeral helper preserves
# build-artifact discipline and matches the pattern used by sibling
# angzarr-*-{lang} repos.
mutation-test: generate-proto
    just _container-ephemeral mutation-test

# Purge the local mutation build cache (.mutants-cache/) — ccache only;
# never holds source.
mutants-purge-cache:
    rm -rf "{{ROOT}}/.mutants-cache"
    @echo "Removed {{ROOT}}/.mutants-cache"

# Create release archive
archive VERSION:
    just _container archive {{VERSION}}

# Idempotent build-artifact cleanup per cross-language convention.
#
# Wipes EVERY transient artifact the C++ toolchain emits so a subsequent
# `just generate-proto-force && just build` produces a clean tree from
# scratch. Safe to run repeatedly; never touches tracked source. Mirrors
# the Java/C#/Go pilot `just clean` recipes.
clean:
    # Some past mull/clang invocations have chmodded files inside the
    # build tree to read-only (-r--r--r--), which then makes `rm -rf`
    # fail half-way. Restore writability before the wipe. `|| true` so
    # this stays no-op on a clean working tree where no build dirs exist.
    find "{{ROOT}}" -maxdepth 1 \( -name 'build' -o -name 'build-*' -o -name 'cmake-build-*' \) \
        -exec chmod -R u+w {} + 2>/dev/null || true
    # Primary build tree + variant trees from mutation/sanity iterations.
    # `build/` itself is the parent (CMakePresets writes to
    # build/container/ and build/container-mull/); pre-presets trees may
    # still exist as bare build-iter*/build-mull*/build-regen/build-sanity/
    # — the wildcard sweeps them all.
    rm -rf "{{ROOT}}/build" "{{ROOT}}"/build-* "{{ROOT}}"/cmake-build-*
    # CMake debris that can stray to repo root if cmake is invoked there.
    rm -rf "{{ROOT}}/CMakeFiles" "{{ROOT}}/Testing" "{{ROOT}}/.cache"
    rm -f  "{{ROOT}}/CMakeCache.txt" "{{ROOT}}/cmake_install.cmake" \
           "{{ROOT}}/Makefile"
    # compile_commands.json is typically a symlink into build/ — remove
    # if it's a regular file (stale copy).
    test -L "{{ROOT}}/compile_commands.json" || rm -f "{{ROOT}}/compile_commands.json"
    # Object/archive/shared-lib spills at any depth.
    find "{{ROOT}}" -name '*.o' -not -path '*/.git/*' -delete 2>/dev/null || true
    find "{{ROOT}}" -name '*.a' -not -path '*/.git/*' -delete 2>/dev/null || true
    find "{{ROOT}}" -name '*.so' -not -path '*/.git/*' -delete 2>/dev/null || true
    find "{{ROOT}}" -name '*.dylib' -not -path '*/.git/*' -delete 2>/dev/null || true
    # Process crash dumps (per migration: never migrate, always wipe).
    find "{{ROOT}}" -maxdepth 1 -name 'core.*' -delete 2>/dev/null || true
    # Mull / mutation testing artifacts.
    rm -rf "{{ROOT}}/mull-output" "{{ROOT}}/mull-reports"
    find "{{ROOT}}" -maxdepth 1 -name 'mutation_tests-*' -exec rm -rf {} + 2>/dev/null || true
    # Generated proto tree lives under build/ already (handled above),
    # plus stray top-level emit dirs from ad-hoc protoc runs.
    rm -rf "{{ROOT}}/angzarr"
    @echo "[clean] removed build/, build-*/, cmake-build-*/, CMake debris, core dumps, mutation artifacts"

# Check formatting
fmt: generate-proto
    just _container fmt

# Auto-format code
fmt-fix: generate-proto
    just _container fmt-fix

# Cross-language alias — `just check` runs lint + fmt-check.
check: fmt

# Cross-language alias — `just lint` placeholder (C++ uses fmt-check only).
lint: fmt
