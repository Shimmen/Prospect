cmake_minimum_required(VERSION 3.0)
project(Glm)

# Let GLM add its own files
set(GLM_STATIC_LIBRARY_ENABLE yes)
add_subdirectory(glm/)

# Make sure the files are also added as includes under "glm/.."
target_include_directories(glm_static PUBLIC ../glm/)
