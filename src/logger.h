#pragma once
#include <string>

namespace logutil {
    // Create a log file: <dir>/val_ws_dump_YYYYmmdd_HHMMSS.log
    bool init(const std::string& dir);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    std::string log_file_path();
}