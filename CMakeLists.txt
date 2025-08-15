cmake_minimum_required(VERSION 3.10)
project(FoxClipValorant)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_executable(foxclip_valorant
    src/main.cpp
    src/lockfile.cpp
    src/config.cpp
    src/websocket_client.cpp
)

target_include_directories(foxclip_valorant PRIVATE 
    src
    /usr/share/miniconda/include
)

target_link_libraries(foxclip_valorant 
    ${Boost_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)