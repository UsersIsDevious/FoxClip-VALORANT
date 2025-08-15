#include "lockfile.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>

std::optional<Lockfile> read_lockfile() {
    // Load configuration
    Config config = ConfigManager::load_config();
    
    if (config.debug) {
        std::cout << "[DEBUG] Starting lockfile reading process..." << std::endl;
        std::cout << "[DEBUG] Debug mode enabled from config.json" << std::endl;
    }
    
    // Get expanded paths from config
    std::vector<std::string> candidates;
    for (const auto& path : config.lockfile_paths) {
        std::string expanded_path = ConfigManager::expand_environment_variables(path);
        if (!expanded_path.empty()) {
            candidates.push_back(expanded_path);
            if (config.debug) {
                std::cout << "[DEBUG] Added candidate path: " << expanded_path;
                if (expanded_path != path) {
                    std::cout << " (expanded from: " << path << ")";
                }
                std::cout << std::endl;
            }
        }
    }

    if (config.debug) {
        std::cout << "[DEBUG] Checking " << candidates.size() << " candidate paths..." << std::endl;
    }
    
    for(const auto& p : candidates){
        if (config.debug) {
            std::cout << "[DEBUG] Trying to read lockfile from: " << p << std::endl;
        }
        std::ifstream ifs(p, std::ios::in);
        if(!ifs.good()) {
            if (config.debug) {
                std::cout << "[DEBUG] Failed to open file: " << p << std::endl;
            }
            continue;
        }
        
        std::string line; 
        std::getline(ifs, line);
        if(line.empty()) {
            if (config.debug) {
                std::cout << "[DEBUG] File is empty: " << p << std::endl;
            }
            continue;
        }
        
        if (config.debug) {
            std::cout << "[DEBUG] Read line from lockfile: " << line << std::endl;
        }
        
        // name:pid:port:password:protocol
        std::vector<std::string> parts;
        {
            std::string token;
            std::stringstream ss(line);
            while(std::getline(ss, token, ':')) {
                parts.push_back(token);
            }
        }
        
        if (config.debug) {
            std::cout << "[DEBUG] Parsed " << parts.size() << " parts from lockfile" << std::endl;
        }
        
        if(parts.size() >= 5){
            Lockfile lf;
            lf.name = parts[0];
            lf.pid = std::stoi(parts[1]);
            lf.port = std::stoi(parts[2]);
            lf.password = parts[3];
            lf.protocol = parts[4];
            lf.path = p;
            
            if (config.debug) {
                std::cout << "[DEBUG] Successfully parsed lockfile:" << std::endl;
                std::cout << "[DEBUG]   Name: " << lf.name << std::endl;
                std::cout << "[DEBUG]   PID: " << lf.pid << std::endl;
                std::cout << "[DEBUG]   Port: " << lf.port << std::endl;
                std::cout << "[DEBUG]   Password: [REDACTED]" << std::endl;
                std::cout << "[DEBUG]   Protocol: " << lf.protocol << std::endl;
                std::cout << "[DEBUG]   Path: " << lf.path << std::endl;
            }
            
            return lf;
        } else {
            if (config.debug) {
                std::cout << "[DEBUG] Invalid lockfile format, expected at least 5 parts but got " << parts.size() << std::endl;
            }
        }
    }
    
    if (config.debug) {
        std::cout << "[DEBUG] No valid lockfile found in any candidate path" << std::endl;
    }
    return std::nullopt;
}