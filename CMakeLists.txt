cmake_minimum_required(VERSION 3.10)
project(gameboy CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD TYPE Debug)
endif()

set(IMGUI_PATH ${CMAKE_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_PATH}/backends/imgui_impl_sdlrenderer3.cpp
  ${IMGUI_PATH}/misc/cpp/*
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
  ${IMGUI_PATH}
  ${IMGUI_PATH}/backends
  ${IMGUI_PATH}/misc/cpp
)

add_executable(${PROJECT_NAME} src/main.cpp)

target_sources(${PROJECT_NAME} PRIVATE
  src/gb.cpp
  src/cpu.cpp
  src/config.cpp
  src/bus.cpp
  src/ppu.cpp
  src/mappers/nomap.cpp
  src/mappers/mbc1.cpp
  src/mappers/mbc2.cpp
  src/mappers/mbc3.cpp
)

include(FetchContent)

if (WIN32)
  FetchContent_Declare(
    SDL3_mingw
    URL "https://github.com/libsdl-org/SDL/releases/download/release-3.4.14/SDL3-devel-3.4.14-mingw.tar.gz"
  )
  FetchContent_MakeAvailable(SDL3_mingw)

  set(SDL3_DIR "${sdl3_mingw_SOURCE_DIR}/x86_64-w64-mingw32")
  target_include_directories(${PROJECT_NAME} PRIVATE "${SDL3_DIR}/include")
  target_link_directories(${PROJECT_NAME} PRIVATE "${SDL3_DIR}/lib")
  target_include_directories(imgui PRIVATE "${SDL3_DIR}/include")
  target_link_directories(imgui PRIVATE "${SDL3_DIR}/lib")

  add_library(nfd STATIC IMPORTED)
  set_target_properties(nfd PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libnfd_win.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/nfd"
    INTERFACE_LINK_LIBRARIES "ole32;uuid;comctl32"
  )

  if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_options(${PROJECT_NAME} PRIVATE
      -static
      -static-libgcc
      -static-libstdc++
    )
  endif()
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
    ${IMGUI_PATH}/misc/cpp
    nfd
  )
  add_library(SDL3_Win SHARED IMPORTED)
  set_target_properties(SDL3_Win PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SDL3_DIR}/include"
    IMPORTED_IMPLIB "${SDL3_DIR}/lib/libSDL3.dll.a"
    IMPORTED_LOCATION "${SDL3_WIN_DIR}/bin/SDL3.dll"
  )
  target_link_libraries(imgui PUBLIC SDL3_Win)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3_Win
    imgui
    nfd
  )
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL3_DIR}/bin/SDL3.dll"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
elseif(UNIX AND NOT APPLE)
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
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
    ${IMGUI_PATH}/misc/cpp
    ${DBUS_INCLUDE_DIRS}
    nfd
  )
  target_link_libraries(imgui PUBLIC SDL3::SDL3)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    imgui
    nfd
  )
endif()
