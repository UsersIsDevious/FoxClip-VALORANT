#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

using nlohmann::json;

const std::string ConfigManager::CONFIG_FILE_PATH = "config.json";

Config ConfigManager::get_default_config() {
    Config config;
    config.debug = true;
    config.enable_websocket = true;  // Enable WebSocket by default since it's now mandatory
    
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
        // If the file doesn't exist, create a default one
        Config def = get_default_config();
        save_config(def);
        return def;
    }

    std::stringstream buf;
    buf << ifs.rdbuf();

    // Robust JSON parse without throwing exceptions
    json j = json::parse(buf.str(), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) {
        std::cerr << "Failed to parse config.json; falling back to defaults." << std::endl;
        return get_default_config();
    }

    const Config def = get_default_config();
    Config cfg{};
    cfg.debug = j.value("debug", def.debug);
    cfg.enable_websocket = j.value("enable_websocket", def.enable_websocket);

    // lockfile_paths
    cfg.lockfile_paths.clear();
    if (j.contains("lockfile_paths") && j["lockfile_paths"].is_array()) {
        for (const auto& v : j["lockfile_paths"]) {
            if (v.is_string()) {
                std::string p = v.get<std::string>();
                cfg.lockfile_paths.push_back(expand_environment_variables(p));
            }
        }
    }
    if (cfg.lockfile_paths.empty()) {
        // Apply defaults if missing or empty
        cfg.lockfile_paths = def.lockfile_paths;
        for (auto& p : cfg.lockfile_paths) {
            p = expand_environment_variables(p);
        }
    }
    return cfg;
}

void ConfigManager::save_config(const Config& config) {
    std::ofstream ofs(CONFIG_FILE_PATH);
    if (!ofs.good()) {
        std::cerr << "Failed to create config file: " << CONFIG_FILE_PATH << std::endl;
        return;
    }
    json j;
    j["debug"] = config.debug;
    j["enable_websocket"] = config.enable_websocket;
    j["lockfile_paths"] = config.lockfile_paths;
    ofs << j.dump(2) << std::endl; // pretty print with 2 spaces
}