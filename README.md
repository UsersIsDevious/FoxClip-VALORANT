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

### Manual Build

```bash
mkdir build
cd build
cmake ..
make
```

### Automated Builds

Windows executables (.exe files) are automatically built using GitHub Actions:

- **Triggers**: Pushes to `develop` or `master` branches, version tags, and pull requests
- **Build Types**: Both Debug and Release configurations
- **Artifacts**: Download built executables from the GitHub Actions "Artifacts" section
- **Releases**: For version tags (e.g., `v1.0.0`), packaged releases are automatically created

To trigger a build:
1. Push changes to the `develop` branch
2. Create a version tag: `git tag v1.0.0 && git push origin v1.0.0`
3. Use the "Run workflow" button in the GitHub Actions tab

Built executables will be available as downloadable artifacts in the workflow run.

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