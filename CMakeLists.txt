cmake_minimum_required(VERSION 3.1)
project(Prospect)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(deps/)

file(GLOB SOURCES "src/*.c" "src/*.cpp" "src/*.h")

# We sometimes want to compile files as both C++ and GLSL.
# This allows us to access files from the shader directory.
file(GLOB SHADER_CROSS_SOURCES "shaders/*.h")

add_executable(Prospect ${SOURCES} ${SHADER_CROSS_SOURCES})

source_group("Source" FILES ${SOURCES})
source_group("Cross Shaders" FILES ${SHADER_CROSS_SOURCES})

target_include_directories(Prospect PRIVATE "src/")
target_include_directories(Prospect PRIVATE "shaders/")

target_link_libraries(Prospect PRIVATE glfw)
target_link_libraries(Prospect PRIVATE glad)
target_link_libraries(Prospect PRIVATE stb_image)
target_link_libraries(Prospect PRIVATE dear_imgui)
target_link_libraries(Prospect PRIVATE glm_static)
target_link_libraries(Prospect PRIVATE tinyobjloader)
