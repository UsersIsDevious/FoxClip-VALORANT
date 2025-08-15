#pragma once

#include <string>
#include <vector>

struct Config {
    bool debug = true;
    std::vector<std::string> lockfile_paths;
    bool enable_websocket = false;
    bool websocket_auto_start = false;
};

class ConfigManager {
public:
    static Config load_config();
    static void save_config(const Config& config);
    static std::string expand_environment_variables(const std::string& path);
    
private:
    static Config get_default_config();
    static const std::string CONFIG_FILE_PATH;
};