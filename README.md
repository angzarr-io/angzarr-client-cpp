> **⚠️ Notice:** This repository was recently extracted from the [angzarr monorepo](https://github.com/angzarr-io/angzarr) and has not yet been validated as a standalone project. Expect rough edges. See the [Angzarr documentation](https://angzarr.io/) for more information.

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
