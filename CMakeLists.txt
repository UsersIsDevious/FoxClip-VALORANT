cmake_minimum_required(VERSION 3.10)
project(FoxClipValorant)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages for WebSocket functionality
find_package(Boost COMPONENTS system REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "WebSocket dependencies found - WebSocket functionality enabled")

add_executable(foxclip_valorant
    src/main.cpp
    src/lockfile.cpp
    src/config.cpp
    src/websocket_client.cpp
)

target_include_directories(foxclip_valorant PRIVATE 
    src
)

target_link_libraries(foxclip_valorant 
    ${Boost_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)