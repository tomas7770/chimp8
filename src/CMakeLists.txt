cmake_minimum_required(VERSION 3.7)

project(Chimp8)

set(SOURCE_FILES
    Chimp8.cpp
    Chimp8App.cpp
    Chip8.cpp
    Clock.cpp
    Config.cpp
    Platform.cpp
)

add_executable(Chimp8 ${SOURCE_FILES})

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIR})
target_link_libraries(Chimp8 ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARY})
