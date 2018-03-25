cmake_minimum_required(VERSION 3.0)
project(DearImgui)

add_library(dear_imgui src/imgui.cpp src/imgui_draw.cpp src/imgui_demo.cpp)
target_include_directories(dear_imgui PUBLIC include/)
