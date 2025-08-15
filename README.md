# FoxClip-VALORANT

A C++ utility for reading VALORANT/Riot Client lockfile to access the local API.

## Features

- Reads Riot Client lockfile from standard locations
- Parses lockfile format: `name:pid:port:password:protocol`
- Stores data in a structured `Lockfile` class
- Includes detailed debug logging
- Cross-platform support (Windows/Linux)

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```cpp
#include "lockfile.h"

auto lockfile_opt = read_lockfile();
if (lockfile_opt.has_value()) {
    const auto& lockfile = lockfile_opt.value();
    // Use lockfile.port, lockfile.password, etc.
    std::cout << "API available on port: " << lockfile.port << std::endl;
}
```

## Lockfile Location

- **Windows**: `%LocalAppData%\Riot Games\Riot Client\Config\lockfile`
- **Linux**: `/var/lib/riot/lockfile` or `./lockfile`

The lockfile is only available when the Riot Client is running.