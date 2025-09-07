add_executable(GOBLIN main.cpp)

add_subdirectory("util")
add_subdirectory("engine")
add_subdirectory("net")
add_subdirectory("render")

# Point to main.cpp as executable for the project
target_sources(GOBLIN PRIVATE main.cpp)

# libs is an interface, link source code to libraries
target_link_libraries(GOBLIN PRIVATE 
    libs
    util
    engine
    render
    net
)

target_link_options(GOBLIN PRIVATE)

if(WIN32)
  add_custom_command(
    TARGET GOBLIN POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:GOBLIN> $<TARGET_FILE_DIR:GOBLIN>
    COMMAND_EXPAND_LISTS
  )
endif()
