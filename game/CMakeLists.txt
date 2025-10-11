# Author: Jacob Collins
# Note:
# This stackoverflow thread was super helpful in getting modern cmake working:
# https://stackoverflow.com/questions/13703647/how-to-properly-add-include-directories-with-cmake

cmake_minimum_required(VERSION 3.16)
project(GOBLIN VERSION 0.1.0)

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

if(WIN32)
# include(InstallRequiredSystemLibraries)
# option(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()

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

# 
install(DIRECTORY assets DESTINATION Release)
install(TARGETS libs RUNTIME_DEPENDENCIES DESTINATION Release)
# install(TARGETS GOBLIN RUNTIME_DEPENDENCIES DESTINATION Release)

# install(TARGETS GOBLIN libs
#   RUNTIME_DEPENDENCIES DESTINATION Release
#   RUNTIME DESTINATION Release
#   LIBRARY DESTINATION Release
#   ARCHIVE DESTINATION Release
# )
install(TARGETS GOBLIN libs
  RUNTIME DESTINATION Release
  LIBRARY DESTINATION Release
  ARCHIVE DESTINATION Release
)

if (win32)
install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION Release COMPONENT Libraries)
install(FILES $<TARGET_RUNTIME_DLLS:GOBLIN> TYPE BIN DESTINATION Release)


endif()

# Copy assets to mf
# file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/OpenSans-Regular.ttf" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/src/")

file(GLOB DLL_FILES "${CMAKE_BINARY_DIR}/*.dll")
install(FILES ${DLL_FILES}
  DESTINATION Release
)

# Requirements for CPack / making an installer
include(CPack)

set(CPACK_PACKAGE_NAME "GoblinColosseum")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../License.txt")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(WIN32)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_SOURCE_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
endif()

include(GetPrerequisites)
set(CPACK_INSTALL_WITH_DEPS_DEPS_DIRS "Release")
set(CPACK_INSTALL_WITH_DEPS TRUE)