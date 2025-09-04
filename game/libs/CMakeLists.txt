
# Adding SDL_ttf:
# https://github.com/libsdl-org/SDL_ttf/blob/main/docs/INTRO-cmake.md
set(SDLTTF_VENDORED ON)

add_subdirectory(SDL_ttf)
add_subdirectory(SDL)

add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE SDL3_ttf::SDL3_ttf SDL3::SDL3)

