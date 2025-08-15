#include "lockfile.h"
#include "config.h"
#include "websocket_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

std::unique_ptr<RiotWSClient> g_ws_client;

void signal_handler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    if (g_ws_client) {
        g_ws_client->stop();
    }
    exit(0);
}

int main() {
    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "FoxClip-VALORANT Lockfile Reader & WebSocket Client" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Load and display config info
    Config config = ConfigManager::load_config();
    std::cout << "Configuration loaded from config.json" << std::endl;
    std::cout << "Debug mode: " << (config.debug ? "ON" : "OFF") << std::endl;
    std::cout << "WebSocket enabled: " << (config.enable_websocket ? "ON" : "OFF") << std::endl;
    if (config.enable_websocket) {
        std::cout << "WebSocket auto-start: " << (config.websocket_auto_start ? "ON" : "OFF") << std::endl;
    }
    std::cout << std::endl;
    
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
        
        // Start WebSocket client if enabled
        if (config.enable_websocket) {
            std::cout << std::endl << "Starting WebSocket client..." << std::endl;
            g_ws_client = std::make_unique<RiotWSClient>();
            
            if (g_ws_client->start(lockfile)) {
                std::cout << "WebSocket client started successfully!" << std::endl;
                
                if (config.websocket_auto_start) {
                    std::cout << "WebSocket client running in background. Press Ctrl+C to stop." << std::endl;
                    
                    // Keep the program running
                    while (g_ws_client->is_connected()) {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    
                    if (config.debug) {
                        std::cout << "WebSocket client disconnected." << std::endl;
                    }
                } else {
                    std::cout << "WebSocket client started. You can now interact with it programmatically." << std::endl;
                    std::cout << "Current session loop state: " << g_ws_client->get_session_loop_state() << std::endl;
                    
                    // Wait a bit to see some events, then stop
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    std::cout << "Updated session loop state: " << g_ws_client->get_session_loop_state() << std::endl;
                    
                    std::cout << "Stopping WebSocket client..." << std::endl;
                    g_ws_client->stop();
                }
            } else {
                std::cout << "Failed to start WebSocket client." << std::endl;
                return 1;
            }
        } else {
            std::cout << std::endl << "WebSocket functionality disabled in configuration." << std::endl;
        }
    } else {
        std::cout << std::endl << "Failed to read lockfile. Make sure VALORANT/Riot Client is running." << std::endl;
        return 1;
    }
    
    return 0;
}