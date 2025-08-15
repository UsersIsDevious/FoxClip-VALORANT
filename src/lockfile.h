#pragma once

#include <string>
#include <optional>

struct Lockfile {
    std::string name;
    int pid{};
    int port{};
    std::string password;
    std::string protocol;
    std::string path;
};

std::optional<Lockfile> read_lockfile();