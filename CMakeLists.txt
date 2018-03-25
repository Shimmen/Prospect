cmake_minimum_required(VERSION 3.1)
project(Prospect)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(deps/)

file(GLOB SOURCES "src/*.c" "src/*.cpp" "src/*.h")
add_executable(Prospect ${SOURCES})

target_include_directories(Prospect PRIVATE "src/")

target_link_libraries(Prospect PRIVATE glfw)
target_link_libraries(Prospect PRIVATE glad)
target_link_libraries(Prospect PRIVATE stb_image)
target_link_libraries(Prospect PRIVATE dear_imgui)
target_link_libraries(Prospect PRIVATE tinyobjloader)
