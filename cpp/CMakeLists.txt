cmake_minimum_required(VERSION 3.25)
project(SlopUI VERSION 0.2.5 LANGUAGES CXX)
# SlopUI for C++ (the Java version: ../java). slopui: the toolkit (no SDL, no GL); slopui_sdl: the SDL3/OpenGL
# backend; slopui_demo: the test program. Add with add_subdirectory(path/to/SlopUI/cpp) and link slopui (and
# slopui_sdl for the backend), or build here: cmake -S cpp -B build -G Ninja && ninja -C build
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(slopui src/core.cpp src/layout.cpp src/ui.cpp src/widgets.cpp src/text.cpp src/lists.cpp)
target_include_directories(slopui PUBLIC include)
target_compile_options(slopui PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>)

option(SLOPUI_SDL "Build the SDL3/OpenGL backend and the demo" ON)
if(SLOPUI_SDL)
    find_package(SDL3 REQUIRED CONFIG)
    add_library(slopui_sdl backend/gl_painter.cpp backend/ttf_font.cpp backend/sdl_host.cpp backend/stb_impl.cpp)
    target_include_directories(slopui_sdl PUBLIC backend PRIVATE third_party)
    target_link_libraries(slopui_sdl PUBLIC slopui SDL3::SDL3)
    add_executable(slopui_demo demo/main.cpp)
    target_link_libraries(slopui_demo PRIVATE slopui_sdl)
    # the themes next to the program (fonts are not part of SlopUI: see demo/main.cpp)
    add_custom_command(TARGET slopui_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../themes $<TARGET_FILE_DIR:slopui_demo>/themes)
endif()
