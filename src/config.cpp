#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <regex>

const std::string ConfigManager::CONFIG_FILE_PATH = "config.json";

Config ConfigManager::get_default_config() {
    Config config;
    config.debug = true;
    config.enable_websocket = true;  // Enable WebSocket by default since it's now mandatory
    config.websocket_auto_start = false;
    
    // Default lockfile paths with environment variables for cross-platform support
#ifdef _WIN32
    config.lockfile_paths = {
        "%LOCALAPPDATA%\\Riot Games\\Riot Client\\Config\\lockfile",
        "%APPDATA%\\Riot Client\\Config\\lockfile", 
        "C:\\Riot Games\\Riot Client\\Config\\lockfile"
    };
#else
    config.lockfile_paths = {
        "/var/lib/riot/lockfile",
        "./lockfile"
    };
#endif
    
    return config;
}

std::string ConfigManager::expand_environment_variables(const std::string& path) {
    std::string expanded = path;
    
    // Handle Windows environment variables like %LOCALAPPDATA%, %APPDATA%, etc.
    std::regex env_regex(R"(%([^%]+)%)");
    std::smatch match;
    
    while (std::regex_search(expanded, match, env_regex)) {
        std::string env_var = match[1].str();
        const char* env_value = std::getenv(env_var.c_str());
        
        if (env_value) {
            expanded.replace(match.position(), match.length(), env_value);
        } else {
            // If environment variable is not found, remove the placeholder
            expanded.replace(match.position(), match.length(), "");
        }
    }
    
    return expanded;
}

Config ConfigManager::load_config() {
    std::ifstream ifs(CONFIG_FILE_PATH);
    if (!ifs.good()) {
        // Config file doesn't exist, create default one
        Config default_config = get_default_config();
        save_config(default_config);
        return default_config;
    }
    
    // Simple JSON parsing for our specific config structure
    Config config;
    std::string line;
    bool in_paths_array = false;
    
    while (std::getline(ifs, line)) {
        // Trim leading/trailing whitespace but preserve spaces in quoted strings
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            line = line.substr(start, end - start + 1);
        } else if (start == std::string::npos) {
            line = "";
        }
        
        if (line.find("\"debug\":true") != std::string::npos || line.find("\"debug\": true") != std::string::npos) {
            config.debug = true;
        } else if (line.find("\"debug\":false") != std::string::npos || line.find("\"debug\": false") != std::string::npos) {
            config.debug = false;
        } else if (line.find("\"enable_websocket\":true") != std::string::npos || line.find("\"enable_websocket\": true") != std::string::npos) {
            config.enable_websocket = true;
        } else if (line.find("\"enable_websocket\":false") != std::string::npos || line.find("\"enable_websocket\": false") != std::string::npos) {
            config.enable_websocket = false;
        } else if (line.find("\"websocket_auto_start\":true") != std::string::npos || line.find("\"websocket_auto_start\": true") != std::string::npos) {
            config.websocket_auto_start = true;
        } else if (line.find("\"websocket_auto_start\":false") != std::string::npos || line.find("\"websocket_auto_start\": false") != std::string::npos) {
            config.websocket_auto_start = false;
        } else if (line.find("\"lockfile_paths\"") != std::string::npos && line.find("[") != std::string::npos) {
            in_paths_array = true;
        } else if (in_paths_array && line == "]") {
            in_paths_array = false;
        } else if (in_paths_array && line.find("\"") != std::string::npos) {
            // Extract path from quoted string, preserving spaces
            size_t start = line.find("\"");
            size_t end = line.rfind("\"");
            if (start != std::string::npos && end != std::string::npos && start < end) {
                std::string path = line.substr(start + 1, end - start - 1);
                config.lockfile_paths.push_back(path);
            }
        }
    }
    
    // If no paths were loaded, use defaults
    if (config.lockfile_paths.empty()) {
        config = get_default_config();
        config.debug = config.debug; // Keep the debug setting that was read
    }
    
    return config;
}

void ConfigManager::save_config(const Config& config) {
    std::ofstream ofs(CONFIG_FILE_PATH);
    if (!ofs.good()) {
        std::cerr << "Failed to create config file: " << CONFIG_FILE_PATH << std::endl;
        return;
    }
    
    ofs << "{\n";
    ofs << "  \"debug\": " << (config.debug ? "true" : "false") << ",\n";
    ofs << "  \"enable_websocket\": " << (config.enable_websocket ? "true" : "false") << ",\n";
    ofs << "  \"websocket_auto_start\": " << (config.websocket_auto_start ? "true" : "false") << ",\n";
    ofs << "  \"lockfile_paths\": [\n";
    
    for (size_t i = 0; i < config.lockfile_paths.size(); ++i) {
        ofs << "    \"" << config.lockfile_paths[i] << "\"";
        if (i < config.lockfile_paths.size() - 1) {
            ofs << ",";
        }
        ofs << "\n";
    }
    
    ofs << "  ]\n";
    ofs << "}\n";
    
    ofs.close();
}