cmake_minimum_required(VERSION 3.10)
project(FoxClipValorant)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json 3.6.0 REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "Dependencies found:")
message(STATUS "  Boost version: ${Boost_VERSION}")
message(STATUS "  OpenSSL version: ${OPENSSL_VERSION}")

add_executable(foxclip_valorant
    src/main.cpp
    src/config.cpp
    src/lockfile.cpp
    src/websocket_client.cpp
    src/logger.cpp
)

target_include_directories(foxclip_valorant PRIVATE src ${Boost_INCLUDE_DIRS})

target_link_libraries(foxclip_valorant
    PRIVATE
      ${Boost_LIBRARIES}
      OpenSSL::SSL
      OpenSSL::Crypto
      nlohmann_json::nlohmann_json
      Threads::Threads
)
