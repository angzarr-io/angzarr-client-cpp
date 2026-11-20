> **Status: currently out of scope.** This repository is behind the angzarr spec and is incompatible with the current runtime and API. It is not maintained until it is ported or archived.
>
> - **Spec:** it targets angzarr-project `8d60829` (2026-05-19); angzarr-project `main` is now `19f4bd7` (2026-11-20), and the `chore/project-f7362501` branch's bump to `f736250` was never merged.
> - **Known differences:** the sources still include the retired `angzarr/*.pb.h` headers (now under `io/angzarr/`), so the build fails; `main` has been red since the 2026-05-19 submodule bump. The port needs reason keys, removal of `Handling` and `ConflictResolution`, `CommandResult`, the Command | Fact split, descriptor checks and the child handshake (now the coordinator's), and typed refusal messages. See `BEHIND-SPEC.md` on `chore/project-f7362501`.
> - **Use instead:** [angzarr-client-rust](https://github.com/angzarr-io/angzarr-client-rust) and [angzarr-examples-rust](https://github.com/angzarr-io/angzarr-examples-rust) are the current reference implementations; the spec is [angzarr-project](https://github.com/angzarr-io/angzarr-project).

> **⚠️ Out of Date:** This repository is currently out of date. Primary development focus is on the **Rust** and **Python** implementations. The author will get back to updating this, but if you need it sooner, please [open an issue](https://github.com/angzarr-io/angzarr/issues) or contact the author directly.

> **Build status (2026-10-28):** `main` has been red since the 2026-05-19 `angzarr-project` submodule bump, which the sources have not caught up with. The dev-container fix merged on 2026-10-28 (the `justfile` and `lefthook.yml` changes) fixes only the dev-container recursion and worktree mounts; it does not repair the build.

# angzarr-client-cpp

Cpp client library for Angzarr event sourcing framework.

> **Status: intentionally lagging.** This client is temporarily descoped: the project grew too fast in scope, so work is focused on the Rust and Python clients. It is behind the current angzarr stack and may not build against it. It will be brought back up to date once Rust and Python are fully operational.

## Installation

```
# Add to CMakeLists.txt
find_package(angzarr-client REQUIRED)
```

## Usage

```
#include <angzarr/client.h>

auto client = angzarr::Client::Connect("localhost:50051");
```

## License

Licensed under either of Apache License 2.0 or BSD 3-Clause, at your option.

## Development

### Setup

Install git hooks (requires [lefthook](https://github.com/evilmartians/lefthook)):

```bash
lefthook install
```

This configures a pre-commit hook that auto-formats code before each commit.

### Recipes

```bash
just -l              # List all available recipes
just build           # Build the library
just test            # Run tests
just fmt             # Check formatting
just fmt-fix         # Auto-format code
```
