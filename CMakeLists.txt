cmake_minimum_required(VERSION 3.16)
include(GNUInstallDirs)

set(PROJ kanooptorrentd)
project(${PROJ})

find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer Concurrent)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Actual version set by CI at build time by tag
set(PROJ_VERSION "99.99.99")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Build info — git SHA + timestamp baked into a generated header.
# Configured at cmake time, regenerated only if the SHA/timestamp change.
execute_process(
    COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short HEAD
    OUTPUT_VARIABLE BUILD_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT BUILD_GIT_SHA)
    set(BUILD_GIT_SHA "unknown")
endif()
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/buildinfo.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/buildinfo.h"
    @ONLY
)

file(GLOB_RECURSE PROJ_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

qt_add_executable(${PROJ} ${PROJ_SOURCES})

target_compile_options(${PROJ} PRIVATE -Wextra -Wall -Werror)

set_target_properties(${PROJ} PROPERTIES VERSION "${PROJ_VERSION}")
set_target_properties(${PROJ} PROPERTIES SOVERSION 1)

target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

# It's important for the installer that each library here is on its own line
target_link_libraries(${PROJ} PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
    Qt6::Concurrent
    KanoopCommonQt
    KanoopProtocolQt
    KanoopDatabaseQt
    KanoopTorrentQt
)

add_compile_definitions(QT_DEPRECATED_WARNINGS)
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

install(TARGETS ${PROJ} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(TARGETS KanoopCommonQt KanoopDatabaseQt KanoopProtocolQt KanoopTorrentQt
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
