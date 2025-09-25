cmake_minimum_required(VERSION 3.16)
project(TinyFootballClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include headers trong folder "header"
include_directories(${CMAKE_SOURCE_DIR}/header)

# Quét source code trong src/
file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(game ${SRC_FILES})

# SDL2 root path
set(SDL2_ROOT C:/SDL2-2.32.10)

# Include directories
target_include_directories(game PRIVATE
  ${SDL2_ROOT}/i686-w64-mingw32/include
  ${SDL2_ROOT}/i686-w64-mingw32/include/SDL2
  ${SDL2_ROOT}/SDL2_image-2.8.8/i686-w64-mingw32/include
  ${SDL2_ROOT}/SDL2_mixer-2.8.1/i686-w64-mingw32/include
  ${SDL2_ROOT}/SDL2_ttf-2.24.0/i686-w64-mingw32/include
  ${SDL2_ROOT}/SDL2_ttf-2.24.0/i686-w64-mingw32/include/SDL2
)

# Library directories
target_link_directories(game PRIVATE
  ${SDL2_ROOT}/i686-w64-mingw32/lib
  ${SDL2_ROOT}/SDL2_image-2.8.8/i686-w64-mingw32/lib
  ${SDL2_ROOT}/SDL2_mixer-2.8.1/i686-w64-mingw32/lib
  ${SDL2_ROOT}/SDL2_ttf-2.24.0/i686-w64-mingw32/lib
)

# Link các lib bắt buộc
target_link_libraries(game PRIVATE
  mingw32
  SDL2main
  SDL2
  SDL2_image
  SDL2_ttf
  SDL2_mixer
)
