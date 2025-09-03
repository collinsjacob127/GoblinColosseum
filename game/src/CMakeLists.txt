# Point to main.cpp as executable for the project
add_executable(GOBLIN)
target_sources(GOBLIN PRIVATE main.cpp)
# libs is an interface, link source code to libraries
target_link_libraries(GOBLIN PRIVATE libs)
