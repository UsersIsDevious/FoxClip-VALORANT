# FoxClip-VALORANT

A C++ utility for reading VALORANT/Riot Client lockfile to access the local API.

## Features

- **Configuration system**: Uses `config.json` for customizable settings
- **Environment variable support**: Automatically expands variables like `%LOCALAPPDATA%`, `%APPDATA%`
- **Debug mode toggle**: Enable/disable debug logging via configuration
- **Cross-platform paths**: Supports both Windows and Linux lockfile locations
- **Automatic config creation**: Creates default config if none exists
- Parses lockfile format: `name:pid:port:password:protocol`
- Stores data in a structured `Lockfile` class

## Configuration

The application automatically creates a `config.json` file on first run with default settings:

```json
{
  "debug": true,
  "lockfile_paths": [
    "%LOCALAPPDATA%\\Riot Games\\Riot Client\\Config\\lockfile",
    "%APPDATA%\\Riot Client\\Config\\lockfile",
    "C:\\Riot Games\\Riot Client\\Config\\lockfile",
    "/var/lib/riot/lockfile",
    "./lockfile"
  ]
}
```

### Configuration Options

- **`debug`**: Set to `true` to enable detailed debug logging, `false` to disable
- **`lockfile_paths`**: Array of file paths to search for the lockfile
  - Supports environment variables in Windows format (`%VARIABLE%`)
  - Paths are tried in order until a valid lockfile is found

### Environment Variable Support

The following environment variables are automatically expanded:
- `%LOCALAPPDATA%` - Windows local application data folder
- `%APPDATA%` - Windows application data folder
- Any other environment variables in `%NAME%` format

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

## Default Lockfile Locations

- **Windows**: 
  - `%LocalAppData%\Riot Games\Riot Client\Config\lockfile`
  - `%AppData%\Riot Client\Config\lockfile`
  - `C:\Riot Games\Riot Client\Config\lockfile`
- **Linux**: 
  - `/var/lib/riot/lockfile`
  - `./lockfile`

The lockfile is only available when the Riot Client is running.

## Customizing Paths

You can modify the `config.json` file to add or change lockfile search paths. This is useful if:
- Riot Client is installed in a non-standard location
- You want to add additional fallback paths
- You want to prioritize certain paths over others

Example custom configuration:
```json
{
  "debug": false,
  "lockfile_paths": [
    "D:\\Games\\Riot Games\\Riot Client\\Config\\lockfile",
    "%LOCALAPPDATA%\\Riot Games\\Riot Client\\Config\\lockfile",
    "./test_lockfile"
  ]
}
```