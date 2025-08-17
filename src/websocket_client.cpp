#include "websocket_client.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "logger.h"  // 追加

std::string RiotWSClient::now_iso() {
    using clock = std::chrono::system_clock;
    auto t = clock::to_time_t(clock::now());
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%F %T");
    return oss.str();
}

// Base64 decode implementation
int b64_index(char c) {
    if('A' <= c && c <= 'Z') return c - 'A';
    if('a' <= c && c <= 'z') return c - 'a' + 26;
    if('0' <= c && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;
    return -1;
}

std::string RiotWSClient::base64_decode(std::string s) {
    // Remove whitespace
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch){
        return std::isspace(ch);
    }), s.end());
    
    // Pad
    while(s.size() % 4) s.push_back('=');

    std::string out;
    out.reserve(s.size() * 3 / 4);

    for(size_t i = 0; i < s.size(); i += 4) {
        int n0 = b64_index(s[i]);
        int n1 = b64_index(s[i+1]);
        int n2 = (s[i+2] == '=') ? -1 : b64_index(s[i+2]);
        int n3 = (s[i+3] == '=') ? -1 : b64_index(s[i+3]);
        if(n0 < 0 || n1 < 0) break;
        
        unsigned char b0 = (n0 << 2) | ((n1 & 0x30) >> 4);
        out.push_back(static_cast<char>(b0));
        if(n2 >= 0) {
            unsigned char b1 = ((n1 & 0x0F) << 4) | ((n2 & 0x3C) >> 2);
            out.push_back(static_cast<char>(b1));
            if(n3 >= 0) {
                unsigned char b2 = ((n2 & 0x03) << 6) | n3;
                out.push_back(static_cast<char>(b2));
            }
        }
    }
    return out;
}

std::string RiotWSClient::base64_encode(const std::string& s) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for(unsigned char c : s) {
        val = (val << 8) + c;
        valb += 8;
        while(valb >= 0) {
            out.push_back(tbl[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if(valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
    while(out.size() % 4) out.push_back('=');
    return out;
}

std::string RiotWSClient::basic_auth_header(const std::string& user, const std::string& pwd) {
    return "Basic " + base64_encode(user + ":" + pwd);
}

RiotWSClient::RiotWSClient() 
    : ioc_(1),
      ssl_ctx_(ssl::context::tlsv12_client),
      rng_(std::random_device{}()),
      jitter_(0.0, 0.3) {
    // Allow self-signed certificates (for development)
    ssl_ctx_.set_verify_mode(ssl::verify_none);
    config_ = ConfigManager::load_config();
}

RiotWSClient::~RiotWSClient() {
    stop();
}

bool RiotWSClient::start(const Lockfile& lockfile) {
    if(is_running_.load()) {
        if(config_.debug) {
            std::cerr << "[DEBUG] WebSocket client already running" << std::endl;
        }
        return false;
    }
    
    lockfile_ = lockfile;
    is_running_.store(true);
    stop_.store(false);
    
    // 情報系は debug の時のみ端末に表示
    if(config_.debug) {
        std::cerr << "[info] starting WebSocket client for port " << lockfile_.port << std::endl;
    }
    logutil::info("starting WebSocket client for port " + std::to_string(lockfile_.port));
    
    // Start worker thread that handles reconnection
    worker_thread_ = std::thread([this]() {
        for(;;) {
            if(stop_.load()) break;
            try {
                connect_and_stream();
                break; // Normal exit
            } catch(const std::exception& e) {
                // ファイルには ERROR、端末は debug の時のみ（要求: 端末はエラーのみ）
                logutil::error(std::string("WebSocket connection lost: ") + e.what());
                if(config_.debug) {
                    std::cerr << "[warn] WebSocket connection lost: " << e.what() << std::endl;
                }
                try_close();
                if(stop_.load()) break;
                double sleep_for = backoff_ + jitter_(rng_);
                if(sleep_for < 0.1) sleep_for = 0.1;
                if(config_.debug) {
                    std::cerr << "[info] reconnecting in " << sleep_for << "s" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::duration<double>(sleep_for));
                backoff_ = std::min(backoff_ * 2.0, backoff_max_);
            }
        }
        is_running_.store(false);
    });
    return true;
}

void RiotWSClient::stop() {
    if(!is_running_.load()) return;
    
    if(config_.debug) {
        std::cout << "[DEBUG] Stopping WebSocket client" << std::endl;
    }
    
    stop_.store(true);
    try_close();
    
    if(worker_thread_.joinable()) {
        worker_thread_.join();
    }
    if(reader_thr_.joinable()) {
        reader_thr_.join();
    }
    if(pinger_thr_.joinable()) {
        pinger_thr_.join();
    }
    if(watchdog_thr_.joinable()) {
        watchdog_thr_.join();
    }
    
    is_running_.store(false);
}

bool RiotWSClient::is_connected() const {
    return connected_once_.load() && !stop_.load();
}

std::string RiotWSClient::get_session_loop_state() const {
    return loop_state_;
}

void RiotWSClient::connect_and_stream() {
    std::string host = "127.0.0.1";
    std::string port = std::to_string(lockfile_.port);
    auth_header_ = basic_auth_header("riot", lockfile_.password);
    rest_origin_ = host + ":" + port;

    if(config_.debug) {
        std::cerr << "[info] connecting wss://" << rest_origin_ << std::endl;
    }
    logutil::info(std::string("connecting wss://") + rest_origin_);

    tcp::resolver resolver(ioc_);
    beast::error_code ec;
    auto results = resolver.resolve(host, port, ec);
    if(ec) throw beast::system_error(ec);

    ws_stream_ = std::make_unique<ws::stream<ssl::stream<tcp::socket>>>(ioc_, ssl_ctx_);
    auto& stream = *ws_stream_;
    
    auto ep = net::connect(stream.next_layer().next_layer(), results, ec);
    if(ec) throw beast::system_error(ec);

    stream.next_layer().set_verify_mode(ssl::verify_none);
    stream.next_layer().handshake(ssl::stream_base::client, ec);
    if(ec) throw beast::system_error(ec);

    stream.set_option(ws::stream_base::decorator([&](ws::request_type& req){
        req.set(http::field::authorization, auth_header_);
        req.set(http::field::origin, "https://127.0.0.1");
    }));
    stream.set_option(ws::stream_base::timeout::suggested(beast::role_type::client));
    stream.auto_fragment(true);
    stream.read_message_max(64ull * 1024 * 1024);

    stream.handshake(rest_origin_, "/", ec);
    if(ec) throw beast::system_error(ec);

    if(config_.debug) {
        std::cerr << "[info] WebSocket handshake completed" << std::endl;
    }
    logutil::info("WebSocket handshake completed");

    last_rx_ = std::chrono::steady_clock::now();
    rx_counter_.store(0);  // 受信カウンタをリセット

    subscribe_on_json_api_event();
    await_first_event();

    // Resync state via REST（失敗は warn ログ）
    try {
        resync_state_via_rest();
    } catch(const std::exception& e) {
        logutil::error(std::string("Resync via REST failed: ") + e.what());
        if(config_.debug) {
            std::cerr << "[warn] Resync via REST failed: " << e.what() << std::endl;
        }
    }

    stop_.store(false);
    reader_thr_ = std::thread([this]{ recv_loop(); });
    pinger_thr_ = std::thread([this]{ ping_loop(); });
    watchdog_thr_ = std::thread([this]{ silence_watchdog(); });

    reader_thr_.join();
    pinger_thr_.join();
    watchdog_thr_.join();

    throw std::runtime_error("connection ended");
}

void RiotWSClient::subscribe_on_json_api_event() {
    json sub = json::array({5, "OnJsonApiEvent"});
    write_text(sub.dump());
    logutil::info("subscribed: OnJsonApiEvent");
    if(config_.debug) {
        std::cerr << "[info] subscribed: OnJsonApiEvent" << std::endl;
    }
}

void RiotWSClient::await_first_event() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(first_event_timeout_sec_);
    for(;;) {
        if(std::chrono::steady_clock::now() > deadline) {
            throw std::runtime_error("no event within first_event_timeout");
        }
        std::string msg;
        if(read_text_once(msg, true, 200)) {
            try {
                auto arr = json::parse(msg);
                if(arr.is_array() && arr.size() >= 3 && arr[1] == "OnJsonApiEvent") {
                    auto ev = arr[2];
                    connected_once_.store(true);
                    backoff_ = backoff_min_;
                    logutil::info("connected: first event received (uri=" + ev.value("uri", std::string()) + ")");
                    if(config_.debug) {
                        std::cerr << "[info] receiving... (Ctrl+C で終了)" << std::endl;
                    }
                    return;
                }
            } catch(...) {}
        }
    }
}

void RiotWSClient::resync_state_via_rest() {
    // GET https://127.0.0.1:{port}/chat/v4/presences
    json pres = rest_get_json("/chat/v4/presences");
    if(pres.contains("presences") && pres["presences"].is_array()) {
        for(auto& p : pres["presences"]) {
            if(p.contains("private") && p["private"].is_string()) {
                auto b64s = p["private"].get<std::string>();
                auto raw = base64_decode(b64s);
                try {
                    auto j = json::parse(raw);
                    if(j.contains("sessionLoopState")) {
                        loop_state_ = j["sessionLoopState"].get<std::string>();
                    }
                } catch(...) {}
            }
        }
        if(config_.debug) {
            std::cout << "[INFO] Resynced sessionLoopState=" << loop_state_ << std::endl;
        }
    }
}

json RiotWSClient::rest_get_json(const std::string& path) {
    beast::error_code ec;
    tcp::resolver resolver(ioc_);
    auto results = resolver.resolve("127.0.0.1", rest_origin_.substr(rest_origin_.find(':') + 1), ec);
    if(ec) throw beast::system_error(ec);

    ssl::stream<tcp::socket> ssl_stream(ioc_, ssl_ctx_);
    net::connect(ssl_stream.next_layer(), results, ec);
    if(ec) throw beast::system_error(ec);

    ssl_stream.set_verify_mode(ssl::verify_none);
    ssl_stream.handshake(ssl::stream_base::client, ec);
    if(ec) throw beast::system_error(ec);

    http::request<http::string_body> req{http::verb::get, path, 11};
    req.set(http::field::host, rest_origin_);
    req.set(http::field::authorization, auth_header_);
    req.set(http::field::accept, "application/json");

    http::write(ssl_stream, req, ec);
    if(ec) throw beast::system_error(ec);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(ssl_stream, buffer, res, ec);
    if(ec) throw beast::system_error(ec);

    if(res.result() != http::status::ok) {
        throw std::runtime_error("REST status not OK: " + std::to_string((int)res.result()));
    }

    // Shutdown TLS
    beast::error_code ignore_ec;
    ssl_stream.shutdown(ignore_ec);

    return json::parse(res.body());
}

void RiotWSClient::recv_loop() {
    try {
        for(;;) {
            if(stop_.load()) break;
            std::string msg;
            if(!read_text_once(msg, true, 0)) {
                continue;
            }
            last_rx_ = std::chrono::steady_clock::now();
            rx_counter_.fetch_add(1, std::memory_order_relaxed);  // 受信を記録

            // 受信イベントは debug=true の時のみ端末に表示
            if(config_.debug) {
                std::cout << msg << std::endl;
            }
            // ログには常に保存（INFO）
            logutil::info(std::string("message ") + msg);

            // 内部処理＋presence.private の base64 デコードをログに可読出力
            try {
                auto arr = json::parse(msg);
                if(arr.is_array() && arr.size() >= 3 && arr[1] == "OnJsonApiEvent") {
                    auto ev = arr[2];
                    std::string uri = ev.value("uri", "");
                    if(uri == "/chat/v4/presences") {
                        auto& data = ev["data"];
                        if(data.contains("presences")) {
                            for(auto& pr : data["presences"]) {
                                if(pr.contains("private") && pr["private"].is_string()) {
                                    auto raw = base64_decode(pr["private"].get<std::string>());
                                    try {
                                        auto jp = json::parse(raw);
                                        // 見やすい形（インデント）でログファイルに追記
                                        logutil::info(std::string("presence.private.decoded:\n") + jp.dump(2));
                                        if(jp.contains("sessionLoopState")) {
                                            loop_state_ = jp["sessionLoopState"].get<std::string>();
                                            if(config_.debug) {
                                                logutil::info(std::string("update sessionLoopState=") + loop_state_);
                                            }
                                        }
                                    } catch(...) {
                                        // JSON でない場合は生文字列を保存
                                        logutil::info(std::string("presence.private.decoded_raw: ") + raw);
                                    }
                                }
                            }
                        }
                    }
                }
            } catch(...) {}
        }
    } catch(const std::exception& e) {
        logutil::error(std::string("[RECV] Exception: ") + e.what());
        std::cerr << "[error] recv exception: " << e.what() << std::endl;
    }
}

void RiotWSClient::ping_loop() {
    try {
        while(!stop_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(ping_interval_sec_));
            if(stop_.load()) break;

            auto dt = std::chrono::steady_clock::now() - last_rx_;
            if(std::chrono::duration_cast<std::chrono::seconds>(dt).count() > silence_timeout_sec_) {
                // まずプローブで様子見（非ブロッキング）
                if(probe_events(2000)) {
                    // 何か受信できたので続行
                    continue;
                }
                // プローブも不発なら再接続へ
                logutil::info("[PING] silence persists after probe, closing to reconnect");
                if(config_.debug) {
                    std::cerr << "[ping] silence persists, reconnecting..." << std::endl;
                }
                try_close();
                break;
            }
            write_ping();
        }
    } catch(const std::exception& e) {
        logutil::error(std::string("[PING] Exception: ") + e.what());
        std::cerr << "[error] ping exception: " << e.what() << std::endl;
    }
}

void RiotWSClient::silence_watchdog() {
    try {
        while(!stop_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto dt = std::chrono::steady_clock::now() - last_rx_;
            if(std::chrono::duration_cast<std::chrono::seconds>(dt).count() > silence_timeout_sec_) {
                // まずプローブ
                if(probe_events(2000)) {
                    continue;
                }
                logutil::info("[WATCHDOG] silence persists after probe, closing to reconnect");
                if(config_.debug) {
                    std::cerr << "[watchdog] silence persists, reconnecting..." << std::endl;
                }
                try_close();
                break;
            }
        }
    } catch(const std::exception& e) {
        logutil::error(std::string("[WATCHDOG] ") + e.what());
        std::cerr << "[error] watchdog exception: " << e.what() << std::endl;
        try_close();
    }
}

bool RiotWSClient::read_text_once(std::string& out, bool block, int timeoutMs) {
    beast::flat_buffer buffer;
    beast::error_code ec;

    if(!ws_stream_) return false;

    ws_stream_->read(buffer, ec);
    if(ec) {
        throw beast::system_error(ec);
    }
    out = beast::buffers_to_string(buffer.cdata());
    return true;
}

void RiotWSClient::write_text(const std::string& s) {
    if(!ws_stream_) throw std::runtime_error("ws not open");
    beast::error_code ec;
    std::lock_guard<std::mutex> lk(write_mtx_);  // 送信は排他
    ws_stream_->text(true);
    ws_stream_->write(net::buffer(s), ec);
    if(ec) throw beast::system_error(ec);
}

void RiotWSClient::write_ping() {
    if(!ws_stream_) throw std::runtime_error("ws not open");
    beast::error_code ec;
    std::lock_guard<std::mutex> lk(write_mtx_);  // 送信は排他
    ws_stream_->ping(ws::ping_data{}, ec);
    if(ec) throw beast::system_error(ec);
}

// 非ブロッキング・プローブ実装
bool RiotWSClient::probe_events(int wait_ms) {
    if(!ws_stream_) return false;

    uint64_t start_rx = rx_counter_.load(std::memory_order_relaxed);

    bool expected = false;
    if(!probing_.compare_exchange_strong(expected, true)) {
        // 他のプローブが動作中：到着だけ監視
        int waited = 0;
        while(waited < wait_ms) {
            if(rx_counter_.load(std::memory_order_relaxed) > start_rx) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waited += 50;
        }
        return false;
    }

    // このスレッドがプローブを担当
    auto clear_flag = [&]{ probing_.store(false, std::memory_order_relaxed); };

    try {
        // サブスクを再送（OnJsonApiEvent）
        json sub = json::array({5, "OnJsonApiEvent"});
        write_text(sub.dump());
        logutil::info("probe: re-subscribed OnJsonApiEvent");

        int waited = 0;
        while(waited < wait_ms) {
            if(rx_counter_.load(std::memory_order_relaxed) > start_rx) {
                logutil::info("probe: event received");
                clear_flag();
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            waited += 50;
        }
        logutil::info("probe: no events within wait window");
        clear_flag();
        return false;
    } catch(const std::exception& e) {
        logutil::error(std::string("probe: exception: ") + e.what());
        clear_flag();
        return false;
    }
}

void RiotWSClient::try_close() {
    try {
        if (!ws_stream_) return;

        beast::error_code ec;
        {
            // 書き込み中と競合しないよう送信ロックを共有
            std::lock_guard<std::mutex> lk(write_mtx_);
            ws_stream_->close(ws::close_code::normal, ec);
        }
        if (ec) {
            // 一部の TLS 終了では close_notify が無く "stream truncated" が発生する
            // これは正常系として扱い、警告レベルで記録する
            if (ec == ssl::error::stream_truncated) {
                logutil::warn(std::string("try_close: stream truncated (peer closed without close_notify); treated as normal"));
            } else if (ec == ws::error::closed) {
                // 既に閉じている場合は情報として扱う
                logutil::info(std::string("try_close: websocket already closed: ") + ec.message());
            } else {
                // それ以外はエラーとして記録（処理は継続）
                logutil::error(std::string("try_close: close error: ") + ec.message());
            }
        } else {
            logutil::info("try_close: websocket closed");
        }

        // ストリームを破棄
        ws_stream_.reset();
    } catch (const std::exception& e) {
        logutil::error(std::string("try_close: exception: ") + e.what());
    } catch (...) {
        logutil::error("try_close: unknown exception");
    }
}