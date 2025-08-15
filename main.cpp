#include "lockfile.h"
#include <iostream>

int main() {
    std::cout << "FoxClip-VALORANT Lockfile Reader" << std::endl;
    std::cout << "================================" << std::endl;
    
    auto lockfile_opt = read_lockfile();
    
    if (lockfile_opt.has_value()) {
        const auto& lockfile = lockfile_opt.value();
        std::cout << std::endl << "Lockfile successfully loaded!" << std::endl;
        std::cout << "Name: " << lockfile.name << std::endl;
        std::cout << "PID: " << lockfile.pid << std::endl;
        std::cout << "Port: " << lockfile.port << std::endl;
        std::cout << "Password: " << lockfile.password << std::endl;
        std::cout << "Protocol: " << lockfile.protocol << std::endl;
        std::cout << "Source Path: " << lockfile.path << std::endl;
    } else {
        std::cout << std::endl << "Failed to read lockfile. Make sure VALORANT/Riot Client is running." << std::endl;
        return 1;
    }
    
    return 0;
}