> **⚠️ Out of Date:** This repository is currently out of date. Primary development focus is on the **Rust** and **Python** implementations. The author will get back to updating this, but if you need it sooner, please [open an issue](https://github.com/angzarr-io/angzarr/issues) or contact the author directly.

# angzarr-client-cpp

Cpp client library for Angzarr event sourcing framework.

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

BSD-3-Clause

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

