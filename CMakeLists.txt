cmake_minimum_required(VERSION 3.1)

project(
    OneButtonRhythm
    VERSION 0.1
    LANGUAGES C
)

include_directories(src)
add_executable(${PROJECT_NAME} src/audio.c src/drawing.c src/files.c src/gameplay.c src/main.c src/shared.c src/thread.c)

add_subdirectory(
    deps/raylib
    deps/zip
)

target_link_directories( ${PROJECT_NAME}
    PUBLIC deps/raylib/src
    PUBLIC deps/zip/build
)

target_link_libraries(${PROJECT_NAME} raylib zip)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")