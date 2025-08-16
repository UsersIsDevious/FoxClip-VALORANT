#include "lockfile.h"
#include "config.h"
#include "websocket_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <optional>
#include "logger.h"  // 追加

std::unique_ptr<RiotWSClient> g_ws_client;
std::atomic<bool> g_shutdown{false};

void signal_handler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    g_shutdown.store(true);
    if (g_ws_client) {
        g_ws_client->stop();
    }
    exit(0);
}

// Helper function to compare lockfile content
bool lockfiles_equal(const std::optional<Lockfile>& a, const std::optional<Lockfile>& b) {
    if (!a.has_value() && !b.has_value()) return true;
    if (!a.has_value() || !b.has_value()) return false;
    
    const auto& lf_a = a.value();
    const auto& lf_b = b.value();
    
    return lf_a.name == lf_b.name && 
           lf_a.pid == lf_b.pid && 
           lf_a.port == lf_b.port && 
           lf_a.password == lf_b.password && 
           lf_a.protocol == lf_b.protocol;
}

void display_lockfile_info(const Lockfile& lockfile) {
    // 情報表示は stderr に出力
    std::cerr << std::endl << "Lockfile successfully loaded!" << std::endl;
    std::cerr << "Name: " << lockfile.name << std::endl;
    std::cerr << "PID: " << lockfile.pid << std::endl;
    std::cerr << "Port: " << lockfile.port << std::endl;
    std::cerr << "Password: " << lockfile.password << std::endl;
    std::cerr << "Protocol: " << lockfile.protocol << std::endl;
    std::cerr << "Source Path: " << lockfile.path << std::endl;
}

int main(int argc, char** argv) {
    // --log-dir 簡易解析（既定: ./logs）
    std::string log_dir = "logs";
    for(int i=1; i<argc; ++i) {
        std::string a = argv[i];
        if(a == "--log-dir" && i+1 < argc) {
            log_dir = argv[++i];
        }
    }
    if(!logutil::init(log_dir)) {
        std::cerr << "[error] failed to initialize logger directory: " << log_dir << std::endl;
    } else {
        std::cerr << "log file: " << logutil::log_file_path() << std::endl;
        logutil::info("logger initialized");
    }

    // Setup signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // 情報メッセージは stderr
    std::cerr << "FoxClip-VALORANT Lockfile Reader & WebSocket Client" << std::endl;
    std::cerr << "====================================================" << std::endl;
    
    // Load and display config info
    Config config = ConfigManager::load_config();
    std::cerr << "Configuration loaded from config.json" << std::endl;
    std::cerr << "Debug mode: " << (config.debug ? "ON" : "OFF") << std::endl;
    std::cerr << "WebSocket enabled: " << (config.enable_websocket ? "ON" : "OFF") << std::endl;
    std::cerr << std::endl;
    
    std::optional<Lockfile> current_lockfile;
    std::optional<Lockfile> previous_lockfile;
    
    std::cerr << "Starting continuous monitoring. Press Ctrl+C to stop." << std::endl;
    std::cerr << "=====================================================" << std::endl;
    
    while (!g_shutdown.load()) {
        // Read lockfile
        current_lockfile = read_lockfile();
        
        // Check if lockfile changed
        if (!lockfiles_equal(current_lockfile, previous_lockfile)) {
            if (current_lockfile.has_value()) {
                display_lockfile_info(current_lockfile.value());
                
                // If we have a WebSocket client running but lockfile changed, stop it
                if (g_ws_client && g_ws_client->is_connected()) {
                    std::cout << "Lockfile changed, restarting WebSocket connection..." << std::endl;
                    g_ws_client->stop();
                    g_ws_client.reset();
                }
                
                // Start WebSocket client
                if (config.enable_websocket) {
                    std::cerr << std::endl << "Starting WebSocket client..." << std::endl;
                    logutil::info("Starting WebSocket client");
                    g_ws_client = std::make_unique<RiotWSClient>();
                    
                    if (g_ws_client->start(current_lockfile.value())) {
                        std::cerr << "WebSocket client started successfully!" << std::endl;
                        std::cerr << "Current session loop state: " << g_ws_client->get_session_loop_state() << std::endl;
                    } else {
                        std::cerr << "Failed to start WebSocket client." << std::endl;
                        g_ws_client.reset();
                    }
                }
            } else {
                if (previous_lockfile.has_value()) {
                    std::cout << std::endl << "Lockfile no longer available. VALORANT/Riot Client may have stopped." << std::endl;
                    if (g_ws_client) {
                        g_ws_client->stop();
                        g_ws_client.reset();
                    }
                }
            }
            previous_lockfile = current_lockfile;
        }
        
        // If not connected, wait 1 second before trying again
        if (!g_ws_client || !g_ws_client->is_connected()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            // If connected, just wait and let WebSocket handle events
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    return 0;
}