cmake_minimum_required(VERSION 3.16)
include(GNUInstallDirs)

set(PROJ kanooptorrentd)
project(${PROJ})

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Actual version set by CI at build time by tag
set(PROJ_VERSION "99.99.99")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE PROJ_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

qt_add_executable(${PROJ} ${PROJ_SOURCES})

target_compile_options(${PROJ} PRIVATE -Wextra -Wall -Werror)

set_target_properties(${PROJ} PROPERTIES VERSION "${PROJ_VERSION}")
set_target_properties(${PROJ} PROPERTIES SOVERSION 1)

target_include_directories(${PROJ} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# It's important for the installer that each library here is on its own line
target_link_libraries(${PROJ} PRIVATE
    Qt6::Core
    Qt6::Network
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
