cmake_minimum_required(VERSION 3.0)
project(TinyObjLoader)

add_library(tinyobjloader src/tiny_obj_loader.cc)
target_include_directories(tinyobjloader PUBLIC include/)
