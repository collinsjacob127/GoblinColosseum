# Author: Jacob Collins
# Note:
# This stackoverflow thread was super helpful in getting modern cmake working:
# https://stackoverflow.com/questions/13703647/how-to-properly-add-include-directories-with-cmake

cmake_minimum_required(VERSION 3.16)
project(GOBLIN VERSION 1.0)

# Using C++ Version 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# if(WIN32)
# set(CMAKE_CXX_COMPILER "g++")
# endif()

# Where to find notes moving forward: https://cmake.org/cmake/help/latest/guide/tutorial/Installing%20and%20Testing.html
# and https://cmake.org/cmake/help/latest/guide/tutorial/Packaging%20an%20Installer.html

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Prep c++ toolchain .dlls for windows install
# if(WIN32)
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
#   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
# endif()

# Not sure if this is necessary - last thing I tried before bed
# set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS libgcc libstdc++)

# Link SDL and other external libraries
add_subdirectory(libs)

# Link the source code
add_subdirectory(src)

# set(installable_libs libs src)
# install(TARGETS ${installable_libs} DESTINATION )

# 
install(DIRECTORY assets DESTINATION Release)
# install(TARGETS libs RUNTIME_DEPENDENCIES DESTINATION Release)
install(TARGETS GOBLIN RUNTIME_DEPENDENCIES DESTINATION Release)


# Copy assets to mf
# file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/OpenSans-Regular.ttf" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/src/")

# Requirements for CPack / making an installer
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${GOBLIN_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${GOBLIN_VERSION_MINOR}")
if(WIN32)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_SOURCE_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
endif()
include(CPack)
