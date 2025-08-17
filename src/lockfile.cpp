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

    // 直前のロックファイル内容を記憶（関数内 static）
    static bool s_has_prev = false;
    static std::string s_prev_line;
    static std::string s_prev_path;

    // デバッグログはバッファに貯め、内容変化があった時だけ出力
    std::ostringstream dbg;

    // Get expanded paths from config
    std::vector<std::string> candidates;
    for (const auto& path : config.lockfile_paths) {
        std::string expanded_path = ConfigManager::expand_environment_variables(path);
        if (!expanded_path.empty()) {
            candidates.push_back(expanded_path);
            if (config.debug) {
                dbg << "[DEBUG] Added candidate path: " << expanded_path;
                if (expanded_path != path) {
                    dbg << " (expanded from: " << path << ")";
                }
                dbg << "\n";
            }
        }
    }

    if (config.debug) {
        dbg << "[DEBUG] Checking " << candidates.size() << " candidate paths...\n";
    }

    for (const auto& p : candidates) {
        std::ifstream ifs(p);
        if (!ifs.good()) {
            continue;
        }
        std::string line;
        std::getline(ifs, line);

        // name:pid:port:password:protocol
        std::vector<std::string> parts;
        {
            std::string token;
            std::stringstream ss(line);
            while (std::getline(ss, token, ':')) parts.push_back(token);
        }

        if (parts.size() >= 5) {
            Lockfile lf;
            lf.name = parts[0];
            lf.pid = std::stoi(parts[1]);
            lf.port = std::stoi(parts[2]);
            lf.password = parts[3];
            lf.protocol = parts[4];
            lf.path = p;

            // 変更検知
            bool changed = (!s_has_prev) || (line != s_prev_line) || (p != s_prev_path);

            if (config.debug && changed) {
                // ここで初めてバッファしていたデバッグログを出力
                std::cout << "[DEBUG] Starting lockfile reading process..." << std::endl;
                std::cout << "[DEBUG] Debug mode enabled from config.json" << std::endl;
                std::cout << dbg.str();
                std::cout << "[DEBUG] Trying to read lockfile from: " << p << std::endl;
                std::cout << "[DEBUG] Read line from lockfile: " << line << std::endl;
                std::cout << "[DEBUG] Parsed " << parts.size() << " parts from lockfile" << std::endl;
                std::cout << "[DEBUG] Successfully parsed lockfile:" << std::endl;
                std::cout << "[DEBUG]   Name: " << lf.name << std::endl;
                std::cout << "[DEBUG]   PID: " << lf.pid << std::endl;
                std::cout << "[DEBUG]   Port: " << lf.port << std::endl;
                std::cout << "[DEBUG]   Password: " << lf.password << std::endl;
                std::cout << "[DEBUG]   Protocol: " << lf.protocol << std::endl;
                std::cout << "[DEBUG]   Path: " << lf.path << std::endl;
            }

            // 前回値を更新
            s_has_prev = true;
            s_prev_line = line;
            s_prev_path = p;

            return lf;
        }
        // 不正形式はスルー（連続呼び出しでスパムしないためログしない）
    }

    // 見つからなかった場合、前回まで存在していたなら「消失」を一度だけ通知
    if (config.debug && s_has_prev) {
        std::cout << "[DEBUG] No valid lockfile found in any candidate path" << std::endl;
    }
    s_has_prev = false;
    s_prev_line.clear();
    s_prev_path.clear();
    return std::nullopt;
}