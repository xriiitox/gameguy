cmake_minimum_required(VERSION 3.10)
project(gameboy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(IMGUI_PATH ${CMAKE_SOURCE_DIR}/imgui)

file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdlrenderer3.cpp
  ${IMGUI_PATH}/misc/cpp/*
)

file(GLOB ROMS *.gb)
file(COPY ${ROMS} DESTINATION .)

add_library(imgui STATIC ${IMGUI_SOURCES})

find_package(SDL3 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)

add_library(nfd STATIC IMPORTED)
set_target_properties(nfd PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libnfd.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/nfd"
  INTERFACE_LINK_LIBRARIES "PkgConfig::DBUS;PkgConfig::WAYLAND"
)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${IMGUI_PATH}
  ${IMGUI_PATH}/backends
  ${IMGUI_PATH}/misc/cpp
  ${DBUS_INCLUDE_DIRS}
  nfd
)

target_include_directories(imgui PUBLIC
  ${IMGUI_PATH}
  ${IMGUI_PATH}/backends
  ${IMGUI_PATH}/misc/cpp
)

target_sources(${PROJECT_NAME} PRIVATE
  src/gb.cpp
  src/cpu.cpp
  src/config.cpp
  src/bus.cpp
  src/ppu.cpp
  src/mappers/nomap.cpp
  src/mappers/mbc1.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  SDL3::SDL3
  imgui
  nfd
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)
