# FoxClip-VALORANT

A C++ utility for reading VALORANT/Riot Client lockfile and optionally connecting to the local WebSocket/REST APIs.

## Features

- Configuration via `config.json`
- Environment variable expansion for paths (e.g., `%LOCALAPPDATA%`, `%APPDATA%`)
- Debug mode toggle
- Cross-platform lockfile paths (Windows/Linux)
- Automatic config creation with sensible defaults
- Optional WebSocket connectivity (Boost.Beast + OpenSSL)
- Event monitoring: subscribes to `OnJsonApiEvent` and tracks sessionLoopState
- Auto-reconnection with backoff and watchdog
- Robust JSON handling using nlohmann::json
- Structured logging to rotating file

## Configuration

The app creates `config.json` on first run:

```json
{
  "debug": true,
  "enable_websocket": false,
  "lockfile_paths": [
    "%LOCALAPPDATA%\\Riot Games\\Riot Client\\Config\\lockfile",
    "%APPDATA%\\Riot Client\\Config\\lockfile",
    "C:\\Riot Games\\Riot Client\\Config\\lockfile",
    "/var/lib/riot/lockfile",
    "./lockfile"
  ]
}
```

Notes:
- Paths support Windows-style environment variables.
- Unknown or missing keys fall back to defaults.
- You can optionally add `"websocket_auto_start": true` if supported by your build.

## Building

Prerequisites:
- CMake 3.10+
- C++17 compiler

Optional WebSocket deps:
- Boost.Beast
- OpenSSL
- nlohmann::json (header-only)

Build without WebSocket (default):

```bash
mkdir build && cd build
cmake ..
make
```

Build with WebSocket:

```bash
mkdir build && cd build
cmake .. -DENABLE_WEBSOCKET=ON
make
```

## Usage

- Basic: run the executable; it reads the lockfile from common locations.
- WebSocket mode: enable in `config.json` with `"enable_websocket": true`.

## Logging

Logs are written to a timestamped file under the working directory. Levels: INFO, WARN, ERROR.

## License

MIT License. See [LICENSE](LICENSE).