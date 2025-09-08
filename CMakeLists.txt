cmake_minimum_required(VERSION 4.1.1)
project(hexen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

include_directories(
    vendor/imgui
    vendor/imgui/backends
)

set(IMGUI_SOURCES
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
)

add_executable(hexen ${SOURCE_FILES} ${IMGUI_SOURCES})
file(GLOB_RECURSE HEADER_DIRS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

foreach(header_path IN LISTS HEADER_DIRS)
    get_filename_component(dir ${header_path} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir})
endforeach()

list(REMOVE_DUPLICATES INCLUDE_DIRS)
target_include_directories(hexen PRIVATE ${INCLUDE_DIRS} /usr/include/freetype2 -I/usr/local/include/freetype2 -I/usr/include/libpng16)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenAL REQUIRED)

target_link_libraries(hexen
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
        dl
        freetype
        OpenAL::OpenAL
)
