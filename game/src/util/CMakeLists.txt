# list(APPEND HEADER_FILES render.cpp render.hpp )

add_library(util "")

# List what files are included in this library
target_sources(util
PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/util.cpp
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/util.hpp
)

target_include_directories(util
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Allow render to access SDL
target_link_libraries(util
PRIVATE
    libs
)
