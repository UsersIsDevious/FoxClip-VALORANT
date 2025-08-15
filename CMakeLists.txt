cmake_minimum_required(VERSION 3.10)
project(FoxClipValorant)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(foxclip_valorant
    src/main.cpp
    src/lockfile.cpp
    src/config.cpp
)

target_include_directories(foxclip_valorant PRIVATE src)