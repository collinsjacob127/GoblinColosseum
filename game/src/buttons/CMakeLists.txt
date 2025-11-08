# list(APPEND HEADER_FILES engine.cpp engine.hpp )

add_library(buttons "")

target_sources(buttons
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/buttons.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/buttons.hpp
    )

target_include_directories(buttons
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Allow render to access SDL
target_link_libraries(buttons
PRIVATE
    libs
    util
)
