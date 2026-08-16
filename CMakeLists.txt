cmake_minimum_required(VERSION 3.10)
project(gameboy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${IMGUI_PATH}
  ${IMGUI_PATH}/backends
  ${IMGUI_PATH}/misc/cpp
)

target_include_directories(imgui PUBLIC
  ${IMGUI_PATH}
  ${IMGUI_PATH}/backends
  ${IMGUI_PATH}/misc/cpp
)

target_sources(${PROJECT_NAME} PRIVATE
  gb.cpp
  mem.cpp
  cpu.cpp
  config.cpp
)

add_subdirectory(${CMAKE_SOURCE_DIR}/nativefiledialog-extended)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 imgui nfd)
target_link_libraries(imgui PUBLIC SDL3::SDL3)
