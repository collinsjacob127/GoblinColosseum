add_executable(GOBLIN main.cpp)

add_subdirectory("util")
add_subdirectory("engine")
add_subdirectory("characters")
add_subdirectory("net")
add_subdirectory("render")

# Point to main.cpp as executable for the project
target_sources(GOBLIN PRIVATE main.cpp)

# libs is an interface, link source code to libraries
target_link_libraries(GOBLIN PRIVATE 
    libs
    util
    engine
    characters
    render
    net
)

target_link_options(GOBLIN PUBLIC)

# Copy DLLs for windows installation
if(WIN32)
  add_custom_command(
    TARGET GOBLIN POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:GOBLIN> $<TARGET_FILE_DIR:GOBLIN>
    COMMAND_EXPAND_LISTS
  )
endif()

add_custom_command(
    TARGET GOBLIN POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets/"
            "${PROJECT_BINARY_DIR}/assets/"
    COMMENT "Copying assets recursively"
)
