# Point to main.cpp as executable for the project
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
# libs is an interface, link source code to libraries
target_link_libraries(${PROJECT_NAME} PRIVATE libs)
