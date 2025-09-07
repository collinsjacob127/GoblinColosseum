# Author: Jacob Collins
# Note:
# This stackoverflow thread was super helpful in getting modern cmake working:
# https://stackoverflow.com/questions/13703647/how-to-properly-add-include-directories-with-cmake

cmake_minimum_required(VERSION 3.16)
project(GOBLIN)

# Using C++ Version 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Link SDL and other external libraries
add_subdirectory(libs)

# Link the source code
add_subdirectory(src)

# Copy assets to mf
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/OpenSans-Regular.ttf" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/src/")
