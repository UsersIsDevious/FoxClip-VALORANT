#pragma once
#include <string>

namespace logutil {
    // ログファイルを <dir>/val_ws_dump_YYYYmmdd_HHMMSS.log で作成
    bool init(const std::string& dir);
    void info(const std::string& msg);
    void error(const std::string& msg);
    std::string log_file_path();
}