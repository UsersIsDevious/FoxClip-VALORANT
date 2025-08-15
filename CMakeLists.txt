cmake_minimum_required(VERSION 3.10)
project(FoxClipValorant)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable WebSocket functionality
option(ENABLE_WEBSOCKET "Enable WebSocket functionality" OFF)

# Find optional packages for WebSocket functionality
if(ENABLE_WEBSOCKET)
    find_package(Boost COMPONENTS system)
    find_package(OpenSSL)
    find_package(Threads)
    
    if(Boost_FOUND AND OpenSSL_FOUND AND Threads_FOUND)
        set(WEBSOCKET_AVAILABLE TRUE)
        message(STATUS "WebSocket dependencies found - WebSocket functionality enabled")
    else()
        set(WEBSOCKET_AVAILABLE FALSE)
        message(WARNING "WebSocket dependencies not found - WebSocket functionality disabled")
    endif()
else()
    set(WEBSOCKET_AVAILABLE FALSE)
    message(STATUS "WebSocket functionality disabled by user choice")
endif()

add_executable(foxclip_valorant
    src/main.cpp
    src/lockfile.cpp
    src/config.cpp
)

# Add WebSocket source files if available
if(WEBSOCKET_AVAILABLE)
    target_sources(foxclip_valorant PRIVATE src/websocket_client.cpp)
    target_compile_definitions(foxclip_valorant PRIVATE WEBSOCKET_ENABLED=1)
endif()

target_include_directories(foxclip_valorant PRIVATE 
    src
)

# Link WebSocket libraries if available
if(WEBSOCKET_AVAILABLE)
    target_link_libraries(foxclip_valorant 
        ${Boost_LIBRARIES}
        OpenSSL::SSL 
        OpenSSL::Crypto
        Threads::Threads
    )
endif()