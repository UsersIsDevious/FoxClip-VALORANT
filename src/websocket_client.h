#pragma once

#include <chrono>
#include <string>
#include <thread>
#include <atomic>
#include <optional>
#include <memory>
#include <random>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>

#include <nlohmann/json.hpp>
#include "lockfile.h"
#include "config.h"

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace http = boost::beast::http;
namespace ws = boost::beast::websocket;
namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

class RiotWSClient {
public:
    RiotWSClient();
    ~RiotWSClient();
    
    // Start WebSocket client with the given lockfile info
    bool start(const Lockfile& lockfile);
    
    // Stop the WebSocket client
    void stop();
    
    // Check if currently connected
    bool is_connected() const;
    
    // Get current session loop state
    std::string get_session_loop_state() const;

private:
    void connect_and_stream();
    void subscribe_on_json_api_event();
    void await_first_event();
    void resync_state_via_rest();
    json rest_get_json(const std::string& path);
    
    void recv_loop();
    void ping_loop();
    void silence_watchdog();
    
    bool read_text_once(std::string& out, bool block, int timeoutMs);
    void write_text(const std::string& s);
    void write_ping();
    void try_close();
    
    // Base64 encoding/decoding helpers
    static std::string base64_decode(std::string s);
    static std::string base64_encode(const std::string& s);
    static std::string basic_auth_header(const std::string& user, const std::string& pwd);
    
    // Utility functions
    static std::string now_iso();
    
private:
    net::io_context ioc_;
    ssl::context ssl_ctx_;
    std::unique_ptr<ws::stream<ssl::stream<tcp::socket>>> ws_stream_;
    
    std::thread worker_thread_;
    std::thread reader_thr_, pinger_thr_, watchdog_thr_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> connected_once_{false};
    std::atomic<bool> is_running_{false};
    
    std::chrono::steady_clock::time_point last_rx_{std::chrono::steady_clock::now()};
    std::string auth_header_;
    std::string rest_origin_;
    std::string loop_state_ = "UNKNOWN";
    
    // Configuration
    Lockfile lockfile_;
    Config config_;
    
    // Timeouts and backoff settings
    int silence_timeout_sec_ = 15;
    int ping_interval_sec_ = 10;
    int first_event_timeout_sec_ = 5;
    double backoff_min_ = 0.5;
    double backoff_max_ = 5.0;
    double backoff_ = 0.5;
    
    // Random number generation for jitter
    std::mt19937 rng_;
    std::uniform_real_distribution<double> jitter_;
};