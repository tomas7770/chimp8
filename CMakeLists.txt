cmake_minimum_required(VERSION 3.7)

project(Chimp8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

add_subdirectory(src)
