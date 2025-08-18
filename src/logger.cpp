#include "logger.h"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace {
    std::mutex mtx;
    std::ofstream ofs;
    std::string g_log_path;

    std::string ts() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
}

bool logutil::init(const std::string& dir) {
    std::lock_guard<std::mutex> lock(mtx);
    try {
        fs::create_directories(dir);
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
    #if defined(_WIN32)
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        std::ostringstream name;
        name << "val_ws_dump_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
        g_log_path = (fs::path(dir) / name.str()).string();
        ofs.close();
        ofs.open(g_log_path, std::ios::out | std::ios::trunc);
        return ofs.good();
    } catch(...) {
        return false;
    }
}

void logutil::info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    if(ofs.good()) ofs << ts() << " INFO " << msg << std::endl;
}

void logutil::warn(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    if(ofs.good()) ofs << ts() << " WARN " << msg << std::endl;
}

void logutil::error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    if(ofs.good()) ofs << ts() << " ERROR " << msg << std::endl;
}

std::string logutil::log_file_path() {
    std::lock_guard<std::mutex> lock(mtx);
    return g_log_path;
}