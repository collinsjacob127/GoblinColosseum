
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Adding SDL_ttf:
# https://github.com/libsdl-org/SDL_ttf/blob/main/docs/INTRO-cmake.md
set(SDLTTF_VENDORED ON)

add_subdirectory(SDL_ttf EXCLUDE_FROM_ALL)
add_subdirectory(SDL_image EXCLUDE_FROM_ALL)

# set(SDL_USE_STATIC_LIBS ON)

add_library(libs INTERFACE)
target_link_libraries(libs INTERFACE SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3::SDL3)

# set(installable_libs SDL3_image SDL3_ttf SDL3)
# install(TARGETS ${installable_libs} DESTINATION Release)

# install(FILES "${SDL3_LIBRARY_DIR}/SDL3.dll" DESTINATION Release)
# install(TARGETS libs DESTINATION Release COMPONENT "Vendored Libraries")