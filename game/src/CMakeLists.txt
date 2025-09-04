add_executable(GOBLIN main.cpp)

add_subdirectory("engine")
add_subdirectory("net")
add_subdirectory("render")

# Point to main.cpp as executable for the project
target_sources(GOBLIN PRIVATE main.cpp)

# libs is an interface, link source code to libraries
target_link_libraries(GOBLIN PRIVATE 
    libs
    engine
    render
    net
)
