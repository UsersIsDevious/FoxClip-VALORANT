#include "lockfile.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>

std::optional<Lockfile> read_lockfile() {
    std::cout << "[DEBUG] Starting lockfile reading process..." << std::endl;
    
#ifdef _WIN32
    char* local = std::getenv("LOCALAPPDATA");
    std::vector<std::string> candidates;
    if(local){
        std::string lockfile_path = std::string(local) + "\\Riot Games\\Riot Client\\Config\\lockfile";
        candidates.emplace_back(lockfile_path);
        std::cout << "[DEBUG] Added candidate path from LOCALAPPDATA: " << lockfile_path << std::endl;
    }
    candidates.emplace_back("C:\\Riot Games\\Riot Client\\Config\\lockfile");
    std::cout << "[DEBUG] Added fallback candidate path: C:\\Riot Games\\Riot Client\\Config\\lockfile" << std::endl;
#else
    // 非Windows環境用（必要なら適宜追加）
    std::vector<std::string> candidates{"/var/lib/riot/lockfile", "./lockfile"};
    std::cout << "[DEBUG] Non-Windows environment detected, using alternative paths" << std::endl;
#endif

    std::cout << "[DEBUG] Checking " << candidates.size() << " candidate paths..." << std::endl;
    
    for(const auto& p : candidates){
        std::cout << "[DEBUG] Trying to read lockfile from: " << p << std::endl;
        std::ifstream ifs(p, std::ios::in);
        if(!ifs.good()) {
            std::cout << "[DEBUG] Failed to open file: " << p << std::endl;
            continue;
        }
        
        std::string line; 
        std::getline(ifs, line);
        if(line.empty()) {
            std::cout << "[DEBUG] File is empty: " << p << std::endl;
            continue;
        }
        
        std::cout << "[DEBUG] Read line from lockfile: " << line << std::endl;
        
        // name:pid:port:password:protocol
        std::vector<std::string> parts;
        {
            std::string token;
            std::stringstream ss(line);
            while(std::getline(ss, token, ':')) {
                parts.push_back(token);
            }
        }
        
        std::cout << "[DEBUG] Parsed " << parts.size() << " parts from lockfile" << std::endl;
        
        if(parts.size() >= 5){
            Lockfile lf;
            lf.name = parts[0];
            lf.pid = std::stoi(parts[1]);
            lf.port = std::stoi(parts[2]);
            lf.password = parts[3];
            lf.protocol = parts[4];
            lf.path = p;
            
            std::cout << "[DEBUG] Successfully parsed lockfile:" << std::endl;
            std::cout << "[DEBUG]   Name: " << lf.name << std::endl;
            std::cout << "[DEBUG]   PID: " << lf.pid << std::endl;
            std::cout << "[DEBUG]   Port: " << lf.port << std::endl;
            std::cout << "[DEBUG]   Password: [REDACTED]" << std::endl;
            std::cout << "[DEBUG]   Protocol: " << lf.protocol << std::endl;
            std::cout << "[DEBUG]   Path: " << lf.path << std::endl;
            
            return lf;
        } else {
            std::cout << "[DEBUG] Invalid lockfile format, expected at least 5 parts but got " << parts.size() << std::endl;
        }
    }
    
    std::cout << "[DEBUG] No valid lockfile found in any candidate path" << std::endl;
    return std::nullopt;
}