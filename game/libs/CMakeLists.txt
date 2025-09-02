add_subdirectory(SDL)

add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE SDL3::SDL3)
