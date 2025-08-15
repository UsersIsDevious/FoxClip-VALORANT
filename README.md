# FoxClip-VALORANT

A C++ utility for reading VALORANT/Riot Client lockfile to access the local API.

## Features

- **Configuration system**: Uses `config.json` for customizable settings
- **Environment variable support**: Automatically expands variables like `%LOCALAPPDATA%`, `%APPDATA%`
- **Debug mode toggle**: Enable/disable debug logging via configuration
- **Cross-platform paths**: Supports both Windows and Linux lockfile locations
- **Automatic config creation**: Creates default config if none exists
- **Optional WebSocket connectivity**: Real-time connection to Riot Client WebSocket API (requires dependencies)
- **Event monitoring**: Subscribe to live events from Riot Client (WebSocket mode)
- **Auto-reconnection**: Handles connection drops with exponential backoff (WebSocket mode)
- **Session state tracking**: Monitors sessionLoopState from presence data (WebSocket mode)
- Parses lockfile format: `name:pid:port:password:protocol`
- Stores data in a structured `Lockfile` class

## Configuration

The application automatically creates a `config.json` file on first run with default settings:

```json
{
  "debug": true,
  "enable_websocket": false,
  "websocket_auto_start": false,
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

- **`debug`**: Enable detailed logging for troubleshooting
- **`enable_websocket`**: Enable WebSocket connectivity to Riot Client
- **`websocket_auto_start`**: Keep WebSocket connection running indefinitely
- **`lockfile_paths`**: List of paths to search for the lockfile

### WebSocket Configuration

**Note**: WebSocket functionality is optional and requires additional dependencies (Boost, OpenSSL). If these dependencies are not available during build time, the application will compile without WebSocket support and display a message when WebSocket is requested.

To enable WebSocket functionality, set `enable_websocket` to `true` in your config.json:

```json
{
  "debug": true,
  "enable_websocket": true,
  "websocket_auto_start": false
}
```

- When `websocket_auto_start` is `false`: Connects briefly for demonstration
- When `websocket_auto_start` is `true`: Maintains persistent connection until Ctrl+C

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

### Prerequisites

**Basic build requirements:**
- CMake 3.10+
- C++17 compiler

**Optional WebSocket dependencies:**
- Boost.Beast (for WebSocket functionality)
- OpenSSL (for TLS encryption)  
- nlohmann::json (header-only JSON library)

### Building without WebSocket support (default)

```bash
mkdir build
cd build
cmake ..
make
```

This builds the basic lockfile reader without WebSocket functionality.

### Building with WebSocket support

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y build-essential cmake libboost-all-dev libssl-dev
```

**Build Steps:**
```bash
mkdir build
cd build
cmake .. -DENABLE_WEBSOCKET=ON
make
```

If the dependencies are not found, the build will automatically fall back to basic functionality without WebSocket support.

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

### Basic Lockfile Reading

```cpp
#include "lockfile.h"

auto lockfile_opt = read_lockfile();
if (lockfile_opt.has_value()) {
    const auto& lockfile = lockfile_opt.value();
    // Use lockfile.port, lockfile.password, etc.
    std::cout << "API available on port: " << lockfile.port << std::endl;
}
```

### WebSocket Client Usage

```cpp
#include "websocket_client.h"
#include "lockfile.h"

auto lockfile_opt = read_lockfile();
if (lockfile_opt.has_value()) {
    RiotWSClient ws_client;
    if (ws_client.start(lockfile_opt.value())) {
        // WebSocket connected and monitoring events
        std::cout << "Session state: " << ws_client.get_session_loop_state() << std::endl;
        
        // Keep running or stop as needed
        ws_client.stop();
    }
}
```

### Command Line Usage

```bash
# Run with WebSocket disabled (default)
./foxclip_valorant

# Enable WebSocket in config.json, then run
./foxclip_valorant
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

## WebSocket Features

The WebSocket client provides real-time connectivity to the Riot Client's local API:

### Connection Details
- **Endpoint**: `wss://127.0.0.1:{port}` (port from lockfile)
- **Authentication**: Basic auth with `riot:{password}` (password from lockfile)
- **Protocol**: WebSocket Secure (WSS) with TLS

### Event Monitoring
- Subscribes to `OnJsonApiEvent` for all API events
- Automatically decodes base64-encoded presence data
- Tracks `sessionLoopState` for game state monitoring
- Real-time updates for game events, chat, etc.

### Resilience Features
- **Auto-reconnection**: Exponential backoff (0.5s to 5s) with jitter
- **Connection monitoring**: Ping/pong with silence detection
- **Graceful shutdown**: Clean disconnection on Ctrl+C
- **Error handling**: Robust error recovery and logging

### Session State Tracking
The client automatically monitors and updates the session loop state by:
1. Initial sync via REST API (`/chat/v4/presences`)
2. Real-time updates from WebSocket events
3. Base64 decoding of private presence data
4. Extraction of `sessionLoopState` for game phase tracking

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