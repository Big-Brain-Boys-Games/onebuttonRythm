cmake_minimum_required(VERSION 3.1)

project(
    OneButtonRhythm
    VERSION 0.1
    LANGUAGES C
)

include_directories(src)
add_executable(${PROJECT_NAME} src/audio.c src/drawing.c src/files.c src/gameplay.c src/main.c src/shared.c src/thread.c)

IF (WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_SEARCH_START_STATIC ON
    LINK_SEARCH_END_STATIC ON
  )
ENDIF()

set(CMAKE_DISABLE_TESTING ON)
set(ZIP_STATIC_PIC "true")
set(RAYLIB_LIBTYPE "STATIC")
add_subdirectory(deps/raylib)
add_subdirectory(deps/zip)





target_link_directories( ${PROJECT_NAME}
    PRIVATE deps/raylib/src
    PRIVATE deps/zip
)

target_link_libraries(${PROJECT_NAME} raylib zip)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")
IF (WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -static-libgcc -static")
ENDIF()
