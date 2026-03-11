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

Apache 2.0
