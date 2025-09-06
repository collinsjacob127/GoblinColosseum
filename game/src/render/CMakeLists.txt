# list(APPEND HEADER_FILES render.cpp render.hpp )

add_library(render "")

# List what files are included in this library
target_sources(render
PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/render.cpp
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/render.hpp
)

target_include_directories(render
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Allow render to access SDL
target_link_libraries(render 
PRIVATE
    libs
    engine
)
