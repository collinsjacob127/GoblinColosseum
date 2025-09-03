cmake_minimum_required(VERSION 3.16)
project(GOBLIN)

# Using C++ Version 23
# set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs)
# Link the source code
add_subdirectory(src)

